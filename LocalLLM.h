#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <tchar.h>

#include "ggml.h"
#include "llama.h"

#include <vector>
#include <string>
#include <QFont>
#include <QFontDatabase>
#include <QtCore/QString>
#include <QtConcurrent/QtConcurrent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QDockWidget>

#include "LocalLLM.HelperFunctions.h"
#include "LocalLLM.HelperFunctions.Qt.h"
#include "LocalLLM.Inference.h"
#include "LocalLLM.Resource.h"
#include "LocalLLM.Tokenization.h"

/**
 * @brief Manages the lifecycle of a llama language model runtime, handling initialization, loading, and cleanup.
 */
struct LocalRuntime {
	llama_model* model = nullptr;

	explicit LocalRuntime(const std::string& modelPathUtf8) {
		llama_backend_init();

		const llama_model_params mp = llama_model_default_params();
		
		try {
			model = llama_model_load_from_file(modelPathUtf8.c_str(), mp);
		} catch (const std::exception& e) {
			qDebug() << "Caught exception:" << e.what();
		} catch (...) {
			qWarning() << "Caught an unknown exception type";
		}

		if (model == nullptr) {
			std::string failedToLoadModelFromPath = "Failed to load model from path: ";
			std::wstring modelPathMessage = stringToWString(failedToLoadModelFromPath.append(modelPathUtf8));

			MessageBoxW(nullptr, modelPathMessage.c_str(), L"Local LLM", MB_ICONERROR);
		}
	}

	~LocalRuntime() {
		if (model) {
			llama_model_free(model);
			model = nullptr;
		}
		llama_backend_free();
	}
};
