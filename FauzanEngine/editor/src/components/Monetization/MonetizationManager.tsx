import React, { useState, useCallback } from 'react';

// =========================================================
// IAP Item Types & Data Models
// =========================================================

export interface IAPItem {
  id: string;
  name: string;
  description: string;
  category: 'consumable' | 'non_consumable' | 'subscription' | 'bundle';
  basePrice: number;
  currency: string;
  icon: string;
  tier: 'free' | 'starter' | 'popular' | 'best_value' | 'premium';
  enabled: boolean;
  // Dynamic pricing
  regionPrices: Record<string, number>;
  abTest?: { variant: string; price: number }[];
  // Bundle contents
  bundleItems?: { itemId: string; quantity: number }[];
  // Subscription
  subscriptionPeriod?: 'weekly' | 'monthly' | 'yearly';
  trialDays?: number;
  // Analytics
  purchaseCount: number;
  revenue: number;
  conversionRate: number;
}

export interface LimitedOffer {
  id: string;
  name: string;
  items: { itemId: string; quantity: number }[];
  discount: number;
  startDate: string;
  endDate: string;
  targetSegment: 'all' | 'new_users' | 'returning' | 'whale' | 'at_risk';
  enabled: boolean;
}

export interface AdPlacement {
  id: string;
  name: string;
  type: 'interstitial' | 'rewarded' | 'banner' | 'native';
  trigger: string;
  frequency: number; // max per session
  cooldownMinutes: number;
  enabled: boolean;
  estimatedCPM: number;
  impressions: number;
  revenue: number;
}

export interface PlayerSegment {
  name: string;
  count: number;
  avgRevenue: number;
  avgSessionLength: number;
  retentionD1: number;
  retentionD7: number;
  retentionD30: number;
  color: string;
}

// =========================================================
// Default Data
// =========================================================

const DEFAULT_IAP_ITEMS: IAPItem[] = [
  {
    id: 'gem_small', name: 'Pouch of Gems', description: '100 Gems',
    category: 'consumable', basePrice: 0.99, currency: 'USD', icon: '💎',
    tier: 'starter', enabled: true, regionPrices: { US: 0.99, ID: 15000, JP: 120, EU: 0.99 },
    purchaseCount: 0, revenue: 0, conversionRate: 0,
  },
  {
    id: 'gem_medium', name: 'Bag of Gems', description: '500 Gems + 50 Bonus',
    category: 'consumable', basePrice: 4.99, currency: 'USD', icon: '💎',
    tier: 'popular', enabled: true, regionPrices: { US: 4.99, ID: 79000, JP: 610, EU: 4.99 },
    purchaseCount: 0, revenue: 0, conversionRate: 0,
  },
  {
    id: 'gem_large', name: 'Chest of Gems', description: '1200 Gems + 200 Bonus',
    category: 'consumable', basePrice: 9.99, currency: 'USD', icon: '💎',
    tier: 'best_value', enabled: true, regionPrices: { US: 9.99, ID: 159000, JP: 1220, EU: 9.99 },
    purchaseCount: 0, revenue: 0, conversionRate: 0,
  },
  {
    id: 'vip_pass', name: 'VIP Pass', description: 'Unlock all premium features',
    category: 'subscription', basePrice: 7.99, currency: 'USD', icon: '👑',
    tier: 'premium', enabled: true, subscriptionPeriod: 'monthly', trialDays: 7,
    regionPrices: { US: 7.99, ID: 129000, JP: 980, EU: 7.49 },
    purchaseCount: 0, revenue: 0, conversionRate: 0,
  },
  {
    id: 'starter_bundle', name: 'Starter Bundle', description: '500 Gems + VIP 7 Days + Rare Skin',
    category: 'bundle', basePrice: 2.99, currency: 'USD', icon: '🎁',
    tier: 'starter', enabled: true, regionPrices: { US: 2.99, ID: 49000, JP: 370, EU: 2.99 },
    bundleItems: [{ itemId: 'gem_medium', quantity: 1 }],
    purchaseCount: 0, revenue: 0, conversionRate: 0,
  },
];

const DEFAULT_AD_PLACEMENTS: AdPlacement[] = [
  { id: 'ad_rewarded_life', name: 'Extra Life (Rewarded)', type: 'rewarded', trigger: 'Game Over', frequency: 3, cooldownMinutes: 5, enabled: true, estimatedCPM: 35, impressions: 0, revenue: 0 },
  { id: 'ad_rewarded_coin', name: 'Double Coins (Rewarded)', type: 'rewarded', trigger: 'Level Complete', frequency: 5, cooldownMinutes: 2, enabled: true, estimatedCPM: 30, impressions: 0, revenue: 0 },
  { id: 'ad_interstitial', name: 'Between Levels', type: 'interstitial', trigger: 'Every 3 Levels', frequency: 10, cooldownMinutes: 3, enabled: true, estimatedCPM: 15, impressions: 0, revenue: 0 },
  { id: 'ad_banner', name: 'Menu Banner', type: 'banner', trigger: 'Main Menu', frequency: 999, cooldownMinutes: 0, enabled: false, estimatedCPM: 5, impressions: 0, revenue: 0 },
];

const PLAYER_SEGMENTS: PlayerSegment[] = [
  { name: 'Whales', count: 120, avgRevenue: 89.50, avgSessionLength: 45, retentionD1: 95, retentionD7: 88, retentionD30: 75, color: '#e04040' },
  { name: 'Dolphins', count: 1800, avgRevenue: 12.30, avgSessionLength: 25, retentionD1: 82, retentionD7: 60, retentionD30: 35, color: '#4a9eff' },
  { name: 'Minnows', count: 8500, avgRevenue: 1.20, avgSessionLength: 12, retentionD1: 65, retentionD7: 30, retentionD30: 12, color: '#4ec949' },
  { name: 'Free Players', count: 45000, avgRevenue: 0, avgSessionLength: 8, retentionD1: 40, retentionD7: 15, retentionD30: 5, color: '#888' },
  { name: 'At Risk', count: 3200, avgRevenue: 3.40, avgSessionLength: 3, retentionD1: 20, retentionD7: 5, retentionD30: 1, color: '#e8a030' },
];

// =========================================================
// Aries Economy Agent - AI Suggestions
// =========================================================

interface AriesSuggestion {
  id: string;
  type: 'pricing' | 'ad' | 'retention' | 'bundle' | 'offer';
  priority: 'high' | 'medium' | 'low';
  title: string;
  description: string;
  expectedImpact: string;
  action?: string;
}

function generateAriesSuggestions(items: IAPItem[], segments: PlayerSegment[]): AriesSuggestion[] {
  const suggestions: AriesSuggestion[] = [];

  // Analyze whale segment
  const whales = segments.find(s => s.name === 'Whales');
  const atRisk = segments.find(s => s.name === 'At Risk');

  if (whales && whales.count > 0) {
    suggestions.push({
      id: 'sug_whale_bundle',
      type: 'bundle',
      priority: 'high',
      title: 'Create Whale-Exclusive Bundle',
      description: `${whales.count} whale players averaging $${whales.avgRevenue.toFixed(2)}/user. Create a $49.99 "Ultimate Bundle" with exclusive content to capture more spend.`,
      expectedImpact: `+$${(whales.count * 15).toLocaleString()} estimated monthly revenue`,
      action: 'Create bundle with exclusive skin + 5000 gems + 30-day VIP',
    });
  }

  if (atRisk && atRisk.count > 500) {
    suggestions.push({
      id: 'sug_retention_offer',
      type: 'retention',
      priority: 'high',
      title: 'Win-Back Campaign for At-Risk Players',
      description: `${atRisk.count.toLocaleString()} players at risk of churning (avg session ${atRisk.avgSessionLength}min). Send a "Come Back" offer with 70% discount on starter bundle.`,
      expectedImpact: `Could recover ${Math.floor(atRisk.count * 0.15).toLocaleString()} players`,
      action: 'Deploy targeted push notification with limited-time 70% discount',
    });
  }

  // Check for missing price tiers
  const hasHighTier = items.some(i => i.basePrice >= 19.99);
  if (!hasHighTier) {
    suggestions.push({
      id: 'sug_high_tier',
      type: 'pricing',
      priority: 'medium',
      title: 'Add High-Value IAP Tier ($19.99+)',
      description: 'Your highest IAP is under $20. Whale players often look for premium options. Add a $19.99 and $49.99 tier to capture high-spend players.',
      expectedImpact: '+15-25% revenue from whale segment',
    });
  }

  // Ad optimization
  suggestions.push({
    id: 'sug_ad_rewarded',
    type: 'ad',
    priority: 'medium',
    title: 'Increase Rewarded Ad Frequency',
    description: 'Rewarded ads have 3x higher eCPM than interstitials and improve player satisfaction. Consider adding more rewarded placement points (daily bonus, shop discount).',
    expectedImpact: '+20% ad revenue with maintained retention',
  });

  // Regional pricing
  suggestions.push({
    id: 'sug_regional',
    type: 'pricing',
    priority: 'low',
    title: 'Optimize Regional Pricing for Indonesia',
    description: 'Indonesian market has high volume but low per-user spend. Consider 30% lower pricing (IDR 10,000 instead of IDR 15,000 for small gem pack) to increase conversion rate.',
    expectedImpact: '+40% conversion rate in ID market',
  });

  // Limited time offer suggestion
  suggestions.push({
    id: 'sug_flash_sale',
    type: 'offer',
    priority: 'medium',
    title: 'Weekend Flash Sale Strategy',
    description: 'Players spend 2.3x more on weekends. Schedule automatic 25% discount every Saturday-Sunday to maximize weekend revenue spikes.',
    expectedImpact: '+30% weekend revenue',
  });

  return suggestions;
}

// =========================================================
// Revenue Projection
// =========================================================

interface RevenueProjection {
  month: string;
  iapRevenue: number;
  adRevenue: number;
  totalRevenue: number;
  users: number;
  arpu: number;
}

function projectRevenue(items: IAPItem[], ads: AdPlacement[], months: number = 12): RevenueProjection[] {
  const projections: RevenueProjection[] = [];
  let users = 5000; // starting users
  const growthRate = 0.15; // 15% monthly growth
  const conversionRate = 0.03; // 3% paying users
  const avgIAPSpend = items.reduce((sum, i) => sum + i.basePrice, 0) / items.length;
  const dailyAdImpressions = 3;
  const avgCPM = ads.filter(a => a.enabled).reduce((sum, a) => sum + a.estimatedCPM, 0) / Math.max(ads.filter(a => a.enabled).length, 1);

  const months_names = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

  for (let i = 0; i < months; i++) {
    const payingUsers = Math.floor(users * conversionRate);
    const iapRevenue = payingUsers * avgIAPSpend * 2.5; // avg 2.5 purchases/month
    const adRevenue = (users * dailyAdImpressions * 30 / 1000) * avgCPM;
    const total = iapRevenue + adRevenue;

    projections.push({
      month: months_names[i % 12],
      iapRevenue: Math.round(iapRevenue),
      adRevenue: Math.round(adRevenue),
      totalRevenue: Math.round(total),
      users: Math.round(users),
      arpu: total / users,
    });

    users *= (1 + growthRate);
  }

  return projections;
}

// =========================================================
// Monetization Manager Dialog Component
// =========================================================

interface MonetizationManagerProps {
  isOpen: boolean;
  onClose: () => void;
  onLog: (msg: string) => void;
}

export function MonetizationManager({ isOpen, onClose, onLog }: MonetizationManagerProps) {
  const [activeTab, setActiveTab] = useState<'iap' | 'ads' | 'analytics' | 'segments' | 'aries'>('iap');
  const [items, setItems] = useState<IAPItem[]>(DEFAULT_IAP_ITEMS);
  const [ads, setAds] = useState<AdPlacement[]>(DEFAULT_AD_PLACEMENTS);
  const [editingItem, setEditingItem] = useState<string | null>(null);
  const [suggestions] = useState<AriesSuggestion[]>(() => generateAriesSuggestions(DEFAULT_IAP_ITEMS, PLAYER_SEGMENTS));
  const [projections] = useState<RevenueProjection[]>(() => projectRevenue(DEFAULT_IAP_ITEMS, DEFAULT_AD_PLACEMENTS));

  const updateItem = useCallback((id: string, updates: Partial<IAPItem>) => {
    setItems(prev => prev.map(item => item.id === id ? { ...item, ...updates } : item));
  }, []);

  const addNewItem = useCallback(() => {
    const newItem: IAPItem = {
      id: `item_${Date.now()}`, name: 'New Item', description: 'Description',
      category: 'consumable', basePrice: 0.99, currency: 'USD', icon: '🎮',
      tier: 'starter', enabled: true, regionPrices: { US: 0.99 },
      purchaseCount: 0, revenue: 0, conversionRate: 0,
    };
    setItems(prev => [...prev, newItem]);
    setEditingItem(newItem.id);
    onLog('[Monetization] New IAP item created');
  }, [onLog]);

  const deleteItem = useCallback((id: string) => {
    setItems(prev => prev.filter(i => i.id !== id));
    onLog('[Monetization] IAP item deleted');
  }, [onLog]);

  if (!isOpen) return null;

  const totalProjectedRevenue = projections.reduce((sum, p) => sum + p.totalRevenue, 0);
  const totalUsers = projections[projections.length - 1]?.users || 0;

  const tabs = [
    { id: 'iap' as const, label: '🛒 IAP Items' },
    { id: 'ads' as const, label: '📺 Ad Placements' },
    { id: 'analytics' as const, label: '📊 Revenue Analytics' },
    { id: 'segments' as const, label: '👥 Player Segments' },
    { id: 'aries' as const, label: '🧠 Aries Suggestions' },
  ];

  return (
    <div style={{
      position: 'fixed', inset: 0, zIndex: 10000,
      background: 'rgba(0,0,0,0.7)', display: 'flex',
      alignItems: 'center', justifyContent: 'center',
    }} onClick={(e) => { if (e.target === e.currentTarget) onClose(); }}>
      <div style={{
        background: '#2a2a2a', borderRadius: 8, width: 900, height: '85vh',
        border: '1px solid #444', overflow: 'hidden', display: 'flex', flexDirection: 'column',
      }}>
        {/* Header */}
        <div style={{
          display: 'flex', justifyContent: 'space-between', alignItems: 'center',
          padding: '12px 16px', borderBottom: '1px solid #444', background: '#1e3a1e',
        }}>
          <div>
            <span style={{ color: '#4ec949', fontWeight: 700, fontSize: 14 }}>💰 Monetization Manager</span>
            <span style={{ color: '#888', fontSize: 11, marginLeft: 12 }}>
              Projected: ${totalProjectedRevenue.toLocaleString()}/year | {totalUsers.toLocaleString()} users
            </span>
          </div>
          <button onClick={onClose} style={{
            background: 'none', border: 'none', color: '#888', fontSize: 18, cursor: 'pointer',
          }}>&times;</button>
        </div>

        {/* Tabs */}
        <div style={{ display: 'flex', borderBottom: '1px solid #444', background: '#2e2e2e' }}>
          {tabs.map(tab => (
            <button key={tab.id} onClick={() => setActiveTab(tab.id)} style={{
              padding: '8px 16px', background: activeTab === tab.id ? '#3a3a3a' : 'transparent',
              border: 'none', borderBottom: activeTab === tab.id ? '2px solid #4ec949' : '2px solid transparent',
              color: activeTab === tab.id ? '#fff' : '#888', cursor: 'pointer', fontSize: 11,
            }}>{tab.label}</button>
          ))}
        </div>

        {/* Content */}
        <div style={{ flex: 1, overflow: 'auto', padding: 16 }}>

          {/* IAP Items Tab */}
          {activeTab === 'iap' && (
            <div>
              <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 12 }}>
                <span style={{ color: '#aaa', fontSize: 12 }}>{items.length} items configured</span>
                <button onClick={addNewItem} style={greenBtnStyle}>+ Add Item</button>
              </div>
              <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
                {items.map(item => (
                  <div key={item.id} style={{
                    background: '#1e1e1e', borderRadius: 6, padding: 12,
                    border: editingItem === item.id ? '1px solid #4ec949' : '1px solid #333',
                    opacity: item.enabled ? 1 : 0.5,
                  }}>
                    <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                      <span style={{ fontSize: 24 }}>{item.icon}</span>
                      <div style={{ flex: 1 }}>
                        {editingItem === item.id ? (
                          <input value={item.name} onChange={(e) => updateItem(item.id, { name: e.target.value })}
                            style={{ ...inputStyle, fontWeight: 700, fontSize: 13 }} />
                        ) : (
                          <div style={{ color: '#ddd', fontWeight: 700, fontSize: 13 }}>{item.name}</div>
                        )}
                        <div style={{ color: '#888', fontSize: 11 }}>{item.description}</div>
                      </div>
                      <div style={{ textAlign: 'right' }}>
                        <div style={{ color: '#4ec949', fontWeight: 700, fontSize: 16 }}>
                          ${item.basePrice.toFixed(2)}
                        </div>
                        <span style={{
                          fontSize: 9, padding: '2px 6px', borderRadius: 3,
                          background: tierColors[item.tier] || '#555', color: '#fff',
                        }}>{item.tier.toUpperCase()}</span>
                      </div>
                      <div style={{ display: 'flex', gap: 4 }}>
                        <button onClick={() => setEditingItem(editingItem === item.id ? null : item.id)}
                          style={smallBtnStyle}>Edit</button>
                        <button onClick={() => updateItem(item.id, { enabled: !item.enabled })}
                          style={smallBtnStyle}>{item.enabled ? 'Disable' : 'Enable'}</button>
                        <button onClick={() => deleteItem(item.id)}
                          style={{ ...smallBtnStyle, color: '#e04040' }}>Del</button>
                      </div>
                    </div>
                    {/* Expanded edit form */}
                    {editingItem === item.id && (
                      <div style={{ marginTop: 12, display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: 8 }}>
                        <div>
                          <label style={labelStyle}>Price (USD)</label>
                          <input type="number" value={item.basePrice} step={0.01}
                            onChange={(e) => updateItem(item.id, { basePrice: parseFloat(e.target.value) || 0 })}
                            style={inputStyle} />
                        </div>
                        <div>
                          <label style={labelStyle}>Category</label>
                          <select value={item.category}
                            onChange={(e) => updateItem(item.id, { category: e.target.value as IAPItem['category'] })}
                            style={inputStyle}>
                            <option value="consumable">Consumable</option>
                            <option value="non_consumable">Non-Consumable</option>
                            <option value="subscription">Subscription</option>
                            <option value="bundle">Bundle</option>
                          </select>
                        </div>
                        <div>
                          <label style={labelStyle}>Tier</label>
                          <select value={item.tier}
                            onChange={(e) => updateItem(item.id, { tier: e.target.value as IAPItem['tier'] })}
                            style={inputStyle}>
                            <option value="free">Free</option>
                            <option value="starter">Starter</option>
                            <option value="popular">Popular</option>
                            <option value="best_value">Best Value</option>
                            <option value="premium">Premium</option>
                          </select>
                        </div>
                        <div style={{ gridColumn: 'span 3' }}>
                          <label style={labelStyle}>Description</label>
                          <input value={item.description}
                            onChange={(e) => updateItem(item.id, { description: e.target.value })}
                            style={inputStyle} />
                        </div>
                        <div style={{ gridColumn: 'span 3' }}>
                          <label style={labelStyle}>Regional Prices</label>
                          <div style={{ display: 'flex', gap: 8 }}>
                            {Object.entries(item.regionPrices).map(([region, price]) => (
                              <div key={region} style={{ display: 'flex', alignItems: 'center', gap: 4 }}>
                                <span style={{ color: '#888', fontSize: 10 }}>{region}:</span>
                                <input type="number" value={price} step={0.01}
                                  onChange={(e) => updateItem(item.id, {
                                    regionPrices: { ...item.regionPrices, [region]: parseFloat(e.target.value) || 0 }
                                  })} style={{ ...inputStyle, width: 80 }} />
                              </div>
                            ))}
                          </div>
                        </div>
                      </div>
                    )}
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Ad Placements Tab */}
          {activeTab === 'ads' && (
            <div>
              <div style={{ marginBottom: 12 }}>
                <span style={{ color: '#aaa', fontSize: 12 }}>{ads.filter(a => a.enabled).length}/{ads.length} placements active</span>
              </div>
              {ads.map(ad => (
                <div key={ad.id} style={{
                  background: '#1e1e1e', borderRadius: 6, padding: 12, marginBottom: 8,
                  border: '1px solid #333', opacity: ad.enabled ? 1 : 0.5,
                }}>
                  <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
                    <span style={{ fontSize: 18 }}>
                      {ad.type === 'rewarded' ? '🎬' : ad.type === 'interstitial' ? '📺' : ad.type === 'banner' ? '🏷️' : '📰'}
                    </span>
                    <div style={{ flex: 1 }}>
                      <div style={{ color: '#ddd', fontWeight: 600, fontSize: 12 }}>{ad.name}</div>
                      <div style={{ color: '#888', fontSize: 10 }}>
                        {ad.type.toUpperCase()} | Trigger: {ad.trigger} | Max {ad.frequency}/session | Cooldown: {ad.cooldownMinutes}min
                      </div>
                    </div>
                    <div style={{ textAlign: 'right' }}>
                      <div style={{ color: '#4a9eff', fontSize: 12 }}>eCPM: ${ad.estimatedCPM}</div>
                    </div>
                    <button onClick={() => setAds(prev => prev.map(a => a.id === ad.id ? { ...a, enabled: !a.enabled } : a))}
                      style={{ ...smallBtnStyle, background: ad.enabled ? '#2a5a2a' : '#444' }}>
                      {ad.enabled ? 'ON' : 'OFF'}
                    </button>
                  </div>
                </div>
              ))}
              <div style={{ marginTop: 16, background: '#1a2a1a', padding: 12, borderRadius: 6, border: '1px solid #2a4a2a' }}>
                <div style={{ color: '#4ec949', fontWeight: 600, fontSize: 12, marginBottom: 4 }}>💡 Aries Tip</div>
                <div style={{ color: '#aaa', fontSize: 11 }}>
                  Rewarded ads generate 3x higher eCPM than interstitials and increase player retention by 20%.
                  Always prefer rewarded format. Keep interstitial frequency low to avoid churn.
                </div>
              </div>
            </div>
          )}

          {/* Revenue Analytics Tab */}
          {activeTab === 'analytics' && (
            <div>
              {/* Summary cards */}
              <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: 12, marginBottom: 16 }}>
                {[
                  { label: 'Projected Annual Revenue', value: `$${totalProjectedRevenue.toLocaleString()}`, color: '#4ec949' },
                  { label: 'Target ($1,000,000)', value: `${Math.min(100, Math.round(totalProjectedRevenue / 10000))}%`, color: '#e8a030' },
                  { label: 'Avg ARPU', value: `$${projections.length > 0 ? projections[projections.length - 1].arpu.toFixed(2) : '0'}`, color: '#4a9eff' },
                  { label: 'Projected Users (12mo)', value: totalUsers.toLocaleString(), color: '#c975e0' },
                ].map((card, i) => (
                  <div key={i} style={{
                    background: '#1e1e1e', borderRadius: 6, padding: 12,
                    border: '1px solid #333', textAlign: 'center',
                  }}>
                    <div style={{ color: '#888', fontSize: 10, marginBottom: 4 }}>{card.label}</div>
                    <div style={{ color: card.color, fontSize: 20, fontWeight: 700 }}>{card.value}</div>
                  </div>
                ))}
              </div>

              {/* Revenue chart (ASCII-style bar chart) */}
              <div style={{ background: '#1e1e1e', borderRadius: 6, padding: 16, border: '1px solid #333' }}>
                <div style={{ color: '#ddd', fontWeight: 600, fontSize: 12, marginBottom: 12 }}>Monthly Revenue Projection</div>
                <div style={{ display: 'flex', alignItems: 'flex-end', gap: 4, height: 200 }}>
                  {projections.map((p, i) => {
                    const maxRev = Math.max(...projections.map(pr => pr.totalRevenue));
                    const iapH = (p.iapRevenue / maxRev) * 180;
                    const adH = (p.adRevenue / maxRev) * 180;
                    return (
                      <div key={i} style={{ flex: 1, display: 'flex', flexDirection: 'column', alignItems: 'center' }}>
                        <div style={{ fontSize: 8, color: '#888', marginBottom: 2 }}>${(p.totalRevenue / 1000).toFixed(0)}k</div>
                        <div style={{ width: '100%', display: 'flex', flexDirection: 'column-reverse' }}>
                          <div style={{ height: iapH, background: '#4ec949', borderRadius: '2px 2px 0 0' }} />
                          <div style={{ height: adH, background: '#4a9eff', borderRadius: '2px 2px 0 0' }} />
                        </div>
                        <div style={{ fontSize: 9, color: '#666', marginTop: 4 }}>{p.month}</div>
                      </div>
                    );
                  })}
                </div>
                <div style={{ display: 'flex', gap: 16, marginTop: 8, justifyContent: 'center' }}>
                  <span style={{ fontSize: 10, color: '#4ec949' }}>■ IAP Revenue</span>
                  <span style={{ fontSize: 10, color: '#4a9eff' }}>■ Ad Revenue</span>
                </div>
              </div>

              {/* Player funnel */}
              <div style={{ background: '#1e1e1e', borderRadius: 6, padding: 16, border: '1px solid #333', marginTop: 12 }}>
                <div style={{ color: '#ddd', fontWeight: 600, fontSize: 12, marginBottom: 12 }}>Player Funnel</div>
                {[
                  { label: 'Installs', count: 55620, pct: 100, color: '#4a9eff' },
                  { label: 'Tutorial Complete', count: 38934, pct: 70, color: '#4ec949' },
                  { label: 'Day 1 Retained', count: 22248, pct: 40, color: '#e8a030' },
                  { label: 'First Purchase', count: 1669, pct: 3, color: '#c975e0' },
                  { label: 'Repeat Purchase', count: 556, pct: 1, color: '#e04040' },
                ].map((step, i) => (
                  <div key={i} style={{ marginBottom: 6 }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 2 }}>
                      <span style={{ color: '#aaa', fontSize: 11 }}>{step.label}</span>
                      <span style={{ color: '#888', fontSize: 10 }}>{step.count.toLocaleString()} ({step.pct}%)</span>
                    </div>
                    <div style={{ height: 8, background: '#333', borderRadius: 4, overflow: 'hidden' }}>
                      <div style={{ height: '100%', width: `${step.pct}%`, background: step.color, borderRadius: 4 }} />
                    </div>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Player Segments Tab */}
          {activeTab === 'segments' && (
            <div>
              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12 }}>
                {PLAYER_SEGMENTS.map((seg, i) => (
                  <div key={i} style={{
                    background: '#1e1e1e', borderRadius: 6, padding: 16,
                    border: `1px solid ${seg.color}40`,
                  }}>
                    <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 8 }}>
                      <span style={{ color: seg.color, fontWeight: 700, fontSize: 14 }}>{seg.name}</span>
                      <span style={{ color: '#888', fontSize: 12 }}>{seg.count.toLocaleString()} players</span>
                    </div>
                    <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 8 }}>
                      <div>
                        <div style={{ color: '#666', fontSize: 9 }}>Avg Revenue</div>
                        <div style={{ color: '#4ec949', fontSize: 14, fontWeight: 600 }}>${seg.avgRevenue.toFixed(2)}</div>
                      </div>
                      <div>
                        <div style={{ color: '#666', fontSize: 9 }}>Avg Session</div>
                        <div style={{ color: '#4a9eff', fontSize: 14, fontWeight: 600 }}>{seg.avgSessionLength}min</div>
                      </div>
                      <div>
                        <div style={{ color: '#666', fontSize: 9 }}>Retention D1/D7/D30</div>
                        <div style={{ color: '#ddd', fontSize: 12 }}>
                          {seg.retentionD1}% / {seg.retentionD7}% / {seg.retentionD30}%
                        </div>
                      </div>
                      <div>
                        <div style={{ color: '#666', fontSize: 9 }}>Segment Revenue</div>
                        <div style={{ color: '#e8a030', fontSize: 14, fontWeight: 600 }}>
                          ${(seg.count * seg.avgRevenue).toLocaleString()}
                        </div>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Aries Suggestions Tab */}
          {activeTab === 'aries' && (
            <div>
              <div style={{
                background: '#1a2a3a', padding: 12, borderRadius: 6, marginBottom: 16,
                border: '1px solid #2a4a6a',
              }}>
                <div style={{ color: '#4a9eff', fontWeight: 700, fontSize: 13 }}>🧠 Aries Economy Agent</div>
                <div style={{ color: '#aaa', fontSize: 11, marginTop: 4 }}>
                  AI-powered suggestions based on player behavior analysis, market data, and revenue optimization.
                  Target: $1,000,000 annual revenue.
                </div>
              </div>

              {suggestions.map(sug => (
                <div key={sug.id} style={{
                  background: '#1e1e1e', borderRadius: 6, padding: 14, marginBottom: 8,
                  border: `1px solid ${sug.priority === 'high' ? '#e04040' : sug.priority === 'medium' ? '#e8a030' : '#444'}40`,
                }}>
                  <div style={{ display: 'flex', alignItems: 'center', gap: 8, marginBottom: 6 }}>
                    <span style={{
                      fontSize: 9, padding: '2px 8px', borderRadius: 3, fontWeight: 700,
                      background: sug.priority === 'high' ? '#e04040' : sug.priority === 'medium' ? '#e8a030' : '#555',
                      color: '#fff',
                    }}>{sug.priority.toUpperCase()}</span>
                    <span style={{
                      fontSize: 9, padding: '2px 8px', borderRadius: 3,
                      background: '#333', color: '#aaa',
                    }}>{sug.type.toUpperCase()}</span>
                    <span style={{ color: '#ddd', fontWeight: 600, fontSize: 12 }}>{sug.title}</span>
                  </div>
                  <div style={{ color: '#aaa', fontSize: 11, marginBottom: 6 }}>{sug.description}</div>
                  <div style={{ color: '#4ec949', fontSize: 11, fontWeight: 600 }}>
                    Expected Impact: {sug.expectedImpact}
                  </div>
                  {sug.action && (
                    <button onClick={() => onLog(`[Aries Economy] Executing: ${sug.action}`)}
                      style={{ ...greenBtnStyle, marginTop: 8, fontSize: 10 }}>
                      Apply Suggestion
                    </button>
                  )}
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

// =========================================================
// Styles
// =========================================================

const tierColors: Record<string, string> = {
  free: '#555', starter: '#4a9eff', popular: '#e8a030', best_value: '#4ec949', premium: '#c975e0',
};

const inputStyle: React.CSSProperties = {
  width: '100%', padding: '6px 8px', background: '#2a2a2a', border: '1px solid #444',
  borderRadius: 3, color: '#ddd', fontSize: 11, outline: 'none', boxSizing: 'border-box',
};

const labelStyle: React.CSSProperties = {
  display: 'block', color: '#888', fontSize: 10, marginBottom: 2,
};

const smallBtnStyle: React.CSSProperties = {
  padding: '4px 8px', background: '#333', border: '1px solid #555',
  borderRadius: 3, color: '#ccc', cursor: 'pointer', fontSize: 10,
};

const greenBtnStyle: React.CSSProperties = {
  padding: '6px 14px', background: '#2a5a2a', border: '1px solid #4ec949',
  borderRadius: 4, color: '#4ec949', cursor: 'pointer', fontSize: 11, fontWeight: 600,
};
