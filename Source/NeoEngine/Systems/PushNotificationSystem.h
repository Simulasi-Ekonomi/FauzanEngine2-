#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace NeoEngine {

struct PushNotification {
    std::string title;
    std::string body;
    std::string icon;
    std::string deepLink;
    int delayMinutes = 0;
    bool repeatable = false;
    int repeatIntervalHours = 24;
};

class PushNotificationSystem {
private:
    std::vector<PushNotification> m_Pending;
    std::vector<PushNotification> m_Sent;
    std::function<void(const PushNotification&)> m_OnSend;

public:
    void ScheduleNotification(const std::string& title, const std::string& body, int delayMinutes = 0) {
        PushNotification notif{title, body, "", "", delayMinutes};
        m_Pending.push_back(notif);
    }

    void ScheduleDailyReward(int hour = 8, int minute = 0) {
        PushNotification notif{
            "Daily Reward!", "Claim your daily reward now! Double gold today!",
            "", "daily_reward", 0, true, 24
        };
        m_Pending.push_back(notif);
    }

    void ScheduleReturnPlayer(int daysOffline) {
        PushNotification notif{
            "We miss you!", "Come back and claim " + std::to_string(daysOffline * 100) + " gold!",
            "", "return_reward", 1, false
        };
        m_Pending.push_back(notif);
    }

    void ScheduleFlashSale(const std::string& item, int discount, int durationHours) {
        PushNotification notif{
            "Flash Sale!", item + " is " + std::to_string(discount) + "% off for " + std::to_string(durationHours) + " hours!",
            "", "shop_sale", 5, true, 6
        };
        m_Pending.push_back(notif);
    }

    void ScheduleEnergyFull() {
        PushNotification notif{"Energy Full!", "Your energy is full! Come back and play!", "", "energy", 0, true, 3};
        m_Pending.push_back(notif);
    }

    void SendNow(const PushNotification& notif) {
        m_Sent.push_back(notif);
        if (m_OnSend) m_OnSend(notif);
    }

    std::vector<PushNotification> GetPending() const { return m_Pending; }
    void ClearPending() { m_Pending.clear(); }
    void SetOnSend(std::function<void(const PushNotification&)> cb) { m_OnSend = cb; }
};

} // namespace NeoEngine
