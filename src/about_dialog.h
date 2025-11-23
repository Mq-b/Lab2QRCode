#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;
class QGridLayout;
class QVBoxLayout;
class QHBoxLayout;
class QFrame;

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    void setVersionInfo(const QString& tag, const QString& hash, const QString& branch, const QString& commitTime,
                        const QString& buildTime);

private slots:
    void onGithubClicked();

private:
    void initUI();
    void loadStyleSheet();
    void addInfoRow(QGridLayout* layout, int row, const QString& label, const QString& value);
    QString formatTime(const QString& timeStr) const;

    // UI components
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLabel* m_authorLabel;
    QLabel* m_githubLabel;
    QPushButton* m_closeButton;
    QPushButton* m_githubButton;

    // Version info
    QString m_tag;
    QString m_hash;
    QString m_branch;
    QString m_commitTime;
    QString m_buildTime;
};

#endif
