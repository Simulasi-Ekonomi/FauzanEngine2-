#pragma once
#include <string>
#include <vector>

class UUIManager {
public:
    void InitUIStack();
    void ShowWidget(std::string WidgetName);
    void RenderUI(); // Dipanggil setiap frame setelah render 3D
private:
    std::vector<std::string> ActiveWidgets;
};