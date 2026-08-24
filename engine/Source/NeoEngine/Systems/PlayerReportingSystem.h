#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace NeoEngine {

struct PlayerReport {
    std::string reportId;
    std::string reporterId, reporterName;
    std::string reportedId, reportedName;
    std::string reason;
    std::string description;
    std::string category; // "cheating", "harassment", "spam", "inappropriate", "bug", "other"
    std::chrono::system_clock::time_point timestamp;
    std::string status; // "pending", "reviewed", "actioned", "dismissed"
    std::string moderatorNotes;
};

class PlayerReportingSystem {
private:
    std::vector<PlayerReport> m_Reports;
    std::unordered_map<std::string, int> m_ReportsAgainst;
    std::unordered_map<std::string, int> m_ReportsBy;
    std::function<void(const PlayerReport&)> m_OnReport;
    std::function<void(const PlayerReport&)> m_OnAction;
    int m_AutoMuteThreshold = 5;
    
public:
    bool SubmitReport(const std::string& reporterId, const std::string& reporterName,
                     const std::string& reportedId, const std::string& reportedName,
                     const std::string& reason, const std::string& category,
                     const std::string& description = "") {
        PlayerReport report;
        report.reportId = "rpt_" + std::to_string(m_Reports.size());
        report.reporterId = reporterId;
        report.reporterName = reporterName;
        report.reportedId = reportedId;
        report.reportedName = reportedName;
        report.reason = reason;
        report.category = category;
        report.description = description;
        report.timestamp = std::chrono::system_clock::now();
        report.status = "pending";
        
        m_Reports.push_back(report);
        m_ReportsAgainst[reportedId]++;
        m_ReportsBy[reporterId]++;
        
        if (m_OnReport) m_OnReport(report);
        
        // Auto action jika melebihi threshold
        if (m_ReportsAgainst[reportedId] >= m_AutoMuteThreshold) {
            report.status = "actioned";
            report.moderatorNotes = "Auto-muted: " + std::to_string(m_ReportsAgainst[reportedId]) + " reports received";
            if (m_OnAction) m_OnAction(report);
        }
        
        return true;
    }
    
    std::vector<PlayerReport> GetReportsAgainst(const std::string& playerId) const {
        std::vector<PlayerReport> result;
        for (auto& r : m_Reports) {
            if (r.reportedId == playerId) result.push_back(r);
        }
        return result;
    }
    
    int GetReportCountAgainst(const std::string& playerId) const {
        auto it = m_ReportsAgainst.find(playerId);
        return it != m_ReportsAgainst.end() ? it->second : 0;
    }
    
    bool ReviewReport(const std::string& reportId, const std::string& action, 
                     const std::string& notes = "") {
        for (auto& r : m_Reports) {
            if (r.reportId == reportId) {
                r.status = action;
                r.moderatorNotes = notes;
                if (m_OnAction) m_OnAction(r);
                return true;
            }
        }
        return false;
    }
    
    void SetAutoMuteThreshold(int threshold) { m_AutoMuteThreshold = threshold; }
    void SetOnReport(std::function<void(const PlayerReport&)> cb) { m_OnReport = cb; }
    void SetOnAction(std::function<void(const PlayerReport&)> cb) { m_OnAction = cb; }
};

} // namespace NeoEngine
