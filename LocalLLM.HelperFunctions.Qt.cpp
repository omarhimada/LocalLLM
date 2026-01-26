#include "LocalLLM.h"

QFont makeCodeFont(const int pt = 12) {
    // Preferred families in order
    const QStringList preferred = {
        "JetBrains Mono",
        "Cascadia Mono",
        "Fira Code",
        "IBM Plex Mono",
        "Consolas"
    };

    QFontDatabase db;
    for (const auto& fam : preferred) {
        if (QFontDatabase::families().contains(fam)) {
            QFont f(fam);
            f.setPointSize(pt);
            return f;
        }
    }

    // Last-resort fallback
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setPointSize(pt);
    return f;
}