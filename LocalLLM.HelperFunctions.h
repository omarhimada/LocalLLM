#pragma once
#include <LocalLLM.h>

/**
 * @brief Gets the directory path where the current executable is located.
 * @return A wide string containing the directory path of the current executable, without a trailing slash.
 */
std::wstring getExeDir();

/**
 * @brief Converts a wide character string to a UTF-8 encoded string.
 * @param w The wide character string to convert.
 * @return A UTF-8 encoded string representation of the input wide string, or an empty string if the input is empty.
 */
std::string wideToUtf8(const std::wstring& w);

/**
 * @brief Constructs an absolute file path by combining the executable's directory with a relative path.
 * @param relative A relative path to append to the executable's directory.
 * @return A QString containing the full absolute path.
 */
QString fromExe(const wchar_t* relative);

/**
 * @brief Converts a UTF-8 encoded string to a wide string.
 * @param str The UTF-8 encoded string to convert.
 * @return A wide string representation of the input string, or an empty wide string if the input is empty.
 */
std::wstring stringToWString(const std::string& str);