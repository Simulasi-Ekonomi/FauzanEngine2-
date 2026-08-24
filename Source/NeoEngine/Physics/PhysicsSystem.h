#pragma once
#include "Core/ECS/EntityManager.h"
#include "World/SpatialGrid.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace NeoEngine {

struct CachedContact {
    EntityID a, b;
    float restLength;
    float normalImpulse;   // warm start
    float frictionImpulse; // warm start
};

class PhysicsSystem {
public:
    void Solve(EntityManager& em, float dt) {
        auto entities = em.Query<PositionComponent, VelocityComponent, ColliderComponent>();
        if (entities.size() < 2) return;

        float* px = em.positions.x.data();
        float* pz = em.positions.z.data();
        float* vx = em.velocities.vx.data();
        float* vz = em.velocities.vz.data();
        float* invMass = em.colliders.invMass.data();
        float* radius = em.colliders.radius.data();

        static SpatialGrid grid(80.0f, 2.0f);
        grid.Clear();
        for (EntityID id : entities) grid.Insert(id, px[id], 0.0f, pz[id]);

        // --- Top-K Contact Selection ---
        std::vector<CachedContact> candidates;
        candidates.reserve(2000);
        grid.ForEachNonEmptyCell([&](int cellIdx, const EntityID* entities, int cnt) {
            grid.ForEachPairInCellAndNeighbors(cellIdx, [&](EntityID a, EntityID b) {
                if (a >= b) return;
                float dx = px[a] - px[b], dz = pz[a] - pz[b];
                float d2 = dx*dx + dz*dz;
                float sumR = radius[a] + radius[b];
                if (d2 < sumR*sumR && d2 > 0.0001f) {
                    float penetration = sumR - sqrtf(d2);
                    candidates.push_back({a, b, sumR, 0.0f, 0.0f});
                }
            });
        });

        const int MAX_CONTACTS = 120;
        if ((int)candidates.size() > MAX_CONTACTS) {
            std::partial_sort(candidates.begin(), candidates.begin() + MAX_CONTACTS, candidates.end(),
                [&](const CachedContact& a, const CachedContact& b) {
                    float dxa = px[a.a]-px[a.b], dza = pz[a.a]-pz[a.b];
                    float dxb = px[b.a]-px[b.b], dzb = pz[b.a]-pz[b.b];
                    float pa = a.restLength - sqrtf(dxa*dxa + dza*dza);
                    float pb = b.restLength - sqrtf(dxb*dxb + dzb*dzb);
                    return pa > pb;
                });
            candidates.resize(MAX_CONTACTS);
        }

        if (candidates.empty()) return;

        // --- Kanonik key + Warm Start ---
        for (auto& c : candidates) {
            if (c.a > c.b) std::swap(c.a, c.b);
            uint64_t key = ((uint64_t)c.a << 32) | c.b;
            auto it = m_ContactCache.find(key);
            if (it != m_ContactCache.end()) {
                c.normalImpulse = it->second.normalImpulse;
                c.frictionImpulse = it->second.frictionImpulse;
            }
        }

        // --- Island Solver (Union-Find) – rebuild hanya jika jumlah entitas berubah ---
        if (m_LastEntityCount != entities.size()) {
            m_LastEntityCount = entities.size();
            int maxID = *std::max_element(entities.begin(), entities.end());
            m_Parent.assign(maxID + 1, -1);
            for (EntityID id : entities) m_Parent[id] = id;
        }
        for (auto& c : candidates) Union(c.a, c.b);

        std::unordered_map<int, std::vector<int>> islands;
        for (EntityID id : entities) islands[Find(id)].push_back(id);

        // --- XPBD compliance ---
        float compliance = 0.0001f / (dt * dt);

        // --- Solve per Island ---
        for (auto& isl : islands) {
            std::vector<CachedContact*> islandContacts;
            for (auto& c : candidates)
                if (Find(c.a) == isl.first && Find(c.b) == isl.first)
                    islandContacts.push_back(&c);
            if (islandContacts.empty()) continue;

            for (int iter = 0; iter < 4; ++iter) {
                for (auto* c : islandContacts) {
                    EntityID a = c->a, b = c->b;
                    float dx = px[a] - px[b], dz = pz[a] - pz[b];
                    float d2 = dx*dx + dz*dz;
                    if (d2 < 0.0001f) continue;
                    float dist = sqrtf(d2);
                    float C = dist - c->restLength;
                    if (fabs(C) < 0.01f) continue;

                    // --- Normal Impulse (XPBD) ---
                    float nx = dx / dist, nz = dz / dist;
                    float wA = invMass[a], wB = invMass[b];
                    float w = wA + wB;
                    float dLambda = (-C - compliance * c->normalImpulse) / (w + compliance);
                    c->normalImpulse += dLambda;

                    float oldAx = px[a], oldAz = pz[a], oldBx = px[b], oldBz = pz[b];
                    px[a] += nx * dLambda * wA; pz[a] += nz * dLambda * wA;
                    px[b] -= nx * dLambda * wB; pz[b] -= nz * dLambda * wB;
                    vx[a] += (px[a] - oldAx) / dt; vz[a] += (pz[a] - oldAz) / dt;
                    vx[b] += (px[b] - oldBx) / dt; vz[b] += (pz[b] - oldBz) / dt;

                    // --- Restitution (velocity reflection) ---
                    float vrelx = vx[b] - vx[a], vrelz = vz[b] - vz[a];
                    float vn = vrelx * nx + vrelz * nz;
                    if (vn < 0) {
                        float restitution = 0.3f;
                        float impulse = -(1.0f + restitution) * vn / w;
                        vx[a] -= impulse * wA * nx; vz[a] -= impulse * wA * nz;
                        vx[b] += impulse * wB * nx; vz[b] += impulse * wB * nz;
                    }

                    // --- Friction (Coulomb, isotropik) ---
                    float tx = vrelx - vn * nx;
                    float tz = vrelz - vn * nz;
                    float tangentSpeed2 = tx*tx + tz*tz;
                    if (tangentSpeed2 > 0.0001f) {
                        float invT = 1.0f / sqrtf(tangentSpeed2);
                        tx *= invT; tz *= invT;
                        float maxFriction = 0.3f * fabs(c->normalImpulse);
                        float df = -sqrtf(tangentSpeed2) / (w + 1e-6f);
                        df = fmax(-maxFriction, fmin(df, maxFriction));
                        c->frictionImpulse += df;
                        vx[a] -= tx * df * wA; vz[a] -= tz * df * wA;
                        vx[b] += tx * df * wB; vz[b] += tz * df * wB;
                    }

                    // Damping ringan
                    vx[a] *= 0.98f; vz[a] *= 0.98f;
                    vx[b] *= 0.98f; vz[b] *= 0.98f;
                }
            }
        }

        // --- Simpan cache untuk frame berikutnya ---
        m_ContactCache.clear();
        for (auto& c : candidates) {
            uint64_t key = ((uint64_t)c.a << 32) | c.b;
            m_ContactCache[key] = c;
        }
    }

    int GetContactCount() const { return (int)m_ContactCache.size(); }

private:
    std::unordered_map<uint64_t, CachedContact> m_ContactCache;
    std::vector<int> m_Parent;
    size_t m_LastEntityCount = 0;

    int Find(int x) {
        if (m_Parent[x] != x) m_Parent[x] = Find(m_Parent[x]);
        return m_Parent[x];
    }
    void Union(int a, int b) { a = Find(a); b = Find(b); if (a != b) m_Parent[b] = a; }
};

} // namespace NeoEngine
