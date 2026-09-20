}

bool ItemSerialTracker::IsSerialDuplicate(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    return it != m_Registry.end() && (it->second.consumed || it->second.contaminated);
}

std::string ItemSerialTracker::GetAuditTrail(const std::string& playerId) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::ostringstream out;
    bool found = false;
    for (const auto& [serial, sn] : m_Registry) {
        if (sn.ownerId != playerId && sn.originalOwnerId != playerId) continue;
        found = true;
        out << serial << " | " << sn.itemType << " | " << sn.itemName
            << " | owner=" << sn.ownerId
            << " | verified=" << (sn.verified ? "1" : "0")
            << " | consumed=" << (sn.consumed ? "1" : "0")
            << " | contaminated=" << (sn.contaminated ? "1" : "0") << '\\n';
        for (const auto& record : sn.ownershipChain) {
            out << "  " << record.playerId << " via " << record.transferMethod
                << " from " << record.previousOwner
                << " hash=" << record.transferHash << '\\n';
        }
    }
    return found ? out.str() : std::string{};
}

std::string ItemSerialTracker::GetOwnershipChain(const std::string& serialNumber) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Registry.find(serialNumber);
    if (it == m_Registry.end()) return {};

    std::ostringstream out;
    for (std::size_t i = 0; i < it->second.ownershipChain.size(); ++i) {