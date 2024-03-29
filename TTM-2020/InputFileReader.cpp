#include "pch.h"
#include "InputFileReader.h"
#include "Error.h"
#include "IdTable.h"

TTM::InputFileReader::InputFileReader()
	: m_fileSize(0), m_linesCount(1), m_ignoredCharsCount(0), m_fileText(""), m_codeTable{ IN_CODE_TABLE }
{	}

void TTM::InputFileReader::read(const char* inFilePath)
{
	std::ifstream inputFile(inFilePath, std::ios::ate);

	if (!inputFile.is_open())
		throw ERROR_THROW(110);

	m_fileText.reserve((size_t)inputFile.tellg());
	inputFile.seekg(std::ios::beg);

	for (int readChar = 0, column = 0; (readChar = inputFile.get()) != EOF; ++m_fileSize)
	{
		++column;
		if (readChar == in::endl)
		{
			++m_linesCount;
			column = 0;
			writeDelimiter();
			m_fileText.push_back(in::endl);
		}
		else if (readChar == in::comment)
		{
			if (inputFile.peek() == in::comment)
			{
				do
				{
					readChar = inputFile.get();
					++m_ignoredCharsCount;
				} while (readChar != in::endl);
				m_fileText.push_back(in::endl);
			}
		}

		switch (m_codeTable[readChar])
		{
		case in::T:
			if (!m_fileText.empty() && m_fileText.back() != in::delimiter && m_codeTable[m_fileText.back()] != in::T)
			{
				m_fileText.push_back(in::delimiter);
			}
			m_fileText.push_back(readChar);
			break;

		case in::F:
			throw ERROR_THROW_IN(111, m_linesCount, column);
			break;

		case in::I:
			++m_ignoredCharsCount;
			break;

		case in::O:
			writeDelimiter();
			m_fileText.push_back(readChar);
			break;

		case in::Q:
			writeDelimiter();
			do
			{
				m_fileText.push_back(readChar);
				readChar = inputFile.get();
				++m_fileSize;
				++column;
			} while (m_codeTable[readChar] != in::Q && column <= TI_STR_MAXSIZE);
			m_fileText.push_back(readChar);
			break;

		case in::S:
			writeDelimiter();
			m_fileText.push_back(readChar);
			m_fileText.push_back(in::delimiter);
			break;

		default:
			if (!m_fileText.empty() && m_fileText.back() != in::delimiter)
			{
				m_fileText.push_back(m_codeTable[readChar]);
			}
			break;
		}
	}

	if (std::count(m_fileText.begin(), m_fileText.end(), '\'') % 2 != 0)
		throw ERROR_THROW(132);

	inputFile.close();
}

void TTM::InputFileReader::writeDelimiter()
{
	if (!m_fileText.empty() && m_fileText.back() != in::delimiter)
	{
		m_fileText.push_back(in::delimiter);
	}
}

std::vector<std::pair<std::string, int>> TTM::InputFileReader::splitStringByDelimiter(std::string s, char delimiter)
{
	std::vector<std::pair<std::string, int>> output;
	output.reserve(std::count_if(s.begin(), s.end(), [&](char c) { return c == delimiter; }) + 1);

	int lineNumber = 1;
	size_t start = 0;
	size_t end = s.find(delimiter);

	while (end != std::string::npos)
	{
		std::string substr = s.substr(start, end - start);
		if (substr.find(in::endl) != std::string::npos)
		{
			++lineNumber;
		}
		else
		{
			output.push_back({ substr, lineNumber });
		}
		start = end + 1;
		end = s.find(delimiter, start);
	}

	std::string last_element = s.substr(start, end);
	if (!last_element.empty() && last_element.find(delimiter) == std::string::npos)
		output.push_back({ last_element, lineNumber });

	return output;
}