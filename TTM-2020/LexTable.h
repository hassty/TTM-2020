#pragma once
#include "IdTable.h"

#pragma region LEXEMES
#define LEX_I32							'n'
#define LEX_STR							's'
#define LEX_ID							'i'
#define LEX_DATATYPE					't'
#define LEX_FUNCTION_CALL				'@'
#define LEX_INTEGER_LITERAL				'1'
#define LEX_STRING_LITERAL				'2'
#define LEX_LITERAL						'l'
#define LEX_FN							'f'
#define LEX_IF							'I'
#define LEX_ELSE						'E'
#define LEX_LET							'd'
#define LEX_RET							'r'
#define LEX_ECHO						'p'
#define LEX_MAIN						'm'
#define LEX_SEMICOLON					';'
#define LEX_COMMA						','
#define LEX_OPENING_CURLY_BRACE			'{'
#define LEX_CLOSING_CURLY_BRACE			'}'
#define LEX_OPENING_PARENTHESIS			'('
#define LEX_CLOSING_PARENTHESIS			')'
#define LEX_PLUS						'+'
#define LEX_MINUS						'-'
#define LEX_ASTERISK					'*'
#define LEX_SLASH						'/'
#define LEX_PERCENT						'%'
#define LEX_ASSIGN						'='
#pragma endregion

namespace TTM
{
	class LexTable
	{
	public:
		struct Entry
		{
			char lexeme;
			int lineNumber;
			int idTableIndex;

			Entry(char lexeme, int lineNumber, int idTableIndex = TI_NULLIDX);
		};

		LexTable(size_t capacity = 0);
		void addEntry(const LexTable::Entry& entry);
		const std::string dumpTable(size_t startIndex = 0, size_t endIndex = 0) const;

		bool declaredFunction() const
		{
			return m_table.size() >= 2 && m_table[m_table.size() - 2].lexeme == LEX_FN;
		}

		bool declaredVariable() const
		{
			return m_table.size() >= 2 && m_table[m_table.size() - 2].lexeme == LEX_LET;
		}

		bool declaredDatatype() const
		{
			return m_table.size() >= 1 && (m_table[m_table.size() - 1].lexeme == LEX_DATATYPE);
		}

		int size() const { return m_table.size(); }

		bool hasLexeme(char lexeme) const;

		int lexemeCount(char lexeme) const;

		Entry& operator[](size_t index)
		{
			return m_table[index];
		}

		const Entry& operator[](size_t index) const
		{
			return m_table[index];
		}

	private:
		std::vector<Entry> m_table;
	};
}