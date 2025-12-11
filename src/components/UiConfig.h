#include <string>

class QApplication;
class QFont;

struct UiConfig {
    std::string font_family;
    std::string font_file;
    bool bold;
};

struct Ui {
  private:
    UiConfig config;

  public:
    Ui();
    std::string getFontFamily() const {
        return config.font_family;
    }
    std::string getFontFile() const {
        return config.font_file;
    }
    bool isBold() const {
        return config.bold;
    }

    static UiConfig loadUiConfig(const std::string &filename);
    static void applyFont(QApplication &app, const Ui &ui);

    // 获取应用程序字体（用于控件）
    static QFont getAppFont();
    static QFont getAppFont(int pointSize);
};