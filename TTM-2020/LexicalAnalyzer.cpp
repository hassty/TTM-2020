#include "pch.h"
#include "LexicalAnalyzer.h"
#include "FST.h"

TTM::LexicalAnalyzer::LexicalAnalyzer(LexTable& lextable, IdTable& idtable)
	: lextable(lextable), idtable(idtable)
{	}

char TTM::LexicalAnalyzer::Tokenize(const std::string& str)
{
	FST::FST nanomachinesSon[] = {
		FST_I32, FST_STR, FST_FN, FST_PARSE_INT, FST_IF, FST_ELSE, FST_CONCAT, FST_LET,
		FST_RET, FST_ECHO, FST_MAIN,
		FST_OPENING_PARENTHESIS, FST_CLOSING_PARENTHESIS, FST_SEMICOLON, FST_COMMA,
		FST_OPENING_CURLY_BRACE, FST_CLOSING_CURLY_BRACE,
		FST_PLUS, FST_MINUS, FST_ASTERISK, FST_SLASH, FST_PERCENT,
		FST_EQUALS, FST_NOT_EQUALS, FST_LESS_OR_EQUALS, FST_GREATER_OR_EQUALS,
		FST_ASSIGN, FST_LESS, FST_GREATER,
		FST_ID, FST_STRING_LITERAL, FST_INTEGER_LITERAL
	};
	const int size = sizeof(nanomachinesSon) / sizeof(nanomachinesSon[0]);
	const char tokens[] = {
		LEX_I32, LEX_STR, LEX_FN, LEX_PARSE_INT, LEX_IF, LEX_ELSE, LEX_CONCAT, LEX_LET,
		LEX_RET, LEX_ECHO, LEX_MAIN,
		LEX_OPENING_PARENTHESIS, LEX_CLOSING_PARENTHESIS, LEX_SEMICOLON, LEX_COMMA,
		LEX_OPENING_CURLY_BRACE, LEX_CLOSING_CURLY_BRACE,
		LEX_PLUS, LEX_MINUS, LEX_ASTERISK, LEX_SLASH, LEX_PERCENT,
		LEX_EQUALS, LEX_NOT_EQUALS, LEX_LESS_OR_EQUALS, LEX_GREATER_OR_EQUALS,
		LEX_ASSIGN, LEX_LESS, LEX_GREATER,
		LEX_ID, LEX_STRING_LITERAL, LEX_INTEGER_LITERAL
	};

	for (int i = 0; i < size; ++i) {
		if (execute(str, nanomachinesSon[i])) {
			return tokens[i];
		}
	}

	return EOF;
}

void TTM::LexicalAnalyzer::Scan(const std::vector<std::pair<std::string, int>>& sourceCode, Logger& log)
{
	using type = it::data_type;
	using id_t = it::id_type;

	std::string currentScope = "";
	std::string previousScope = currentScope;
	std::string lastFunctionName = "";
	id_t idType = id_t::unknown;
	type dataType = type::undefined;

	for (const auto& [name, lineNumber] : sourceCode)
	{
		char token = Tokenize(name);
		if (token == EOF)
		{
			//todo add new error type (output token)
			throw ERROR_THROW_IN(129, lineNumber, -1);
		}

		int idTableIndex = TI_NULLIDX;

		switch (token)
		{
		case LEX_CONCAT:
			idTableIndex = idtable.getIdIndexByName("", name);
			if (idTableIndex == TI_NULLIDX)
			{
				idTableIndex = idtable.addEntry({ name, "", lextable.size(), type::str, id_t::function, "" });
			}
			break;

		case LEX_PARSE_INT:
			idTableIndex = idtable.getIdIndexByName("", name);
			if (idTableIndex == TI_NULLIDX)
			{
				idTableIndex = idtable.addEntry({ name, "", lextable.size(), type::i32, id_t::function, "" });
			}
			break;

		case LEX_MAIN:
			idTableIndex = idtable.getIdIndexByName("", name);
			if (idTableIndex == TI_NULLIDX)
			{
				if (!lextable.declaredFunction())
					throw ERROR_THROW_IN(120, lineNumber, -1);
				if (dataType != type::i32)
					throw ERROR_THROW_IN(121, lineNumber, -1);

				idTableIndex = idtable.addEntry({ name, "", lextable.size(), dataType, id_t::function, "" });
				idType = id_t::unknown;
				dataType = type::undefined;
			}
			else if (lextable.declaredFunction())
			{
				throw ERROR_THROW_IN(131, lineNumber, -1);
			}
			break;

		case LEX_ID:
			idTableIndex = idtable.getIdIndexByName(currentScope, name);
			if (idTableIndex == TI_NULLIDX)
			{
				if (lextable.declaredFunction())
				{
					lastFunctionName = name;
					currentScope = "";
					idType = id_t::function;
				}
				else if (lextable.declaredVariable())
				{
					idType = id_t::variable;
				}
				/*else if (lextable.declaredDatatype())
				{
					idType = id_t::parameter;
				}*/
				else
				{
					throw ERROR_THROW_IN(124, lineNumber, -1);
				}

				if (idType == id_t::unknown)
					throw ERROR_THROW_IN(120, lineNumber, -1);
				if (dataType == type::undefined)
					throw ERROR_THROW_IN(121, lineNumber, -1);

				idTableIndex = idtable.addEntry({ name, currentScope, lextable.size(), dataType, idType, "" });
				idType = id_t::unknown;
				dataType = type::undefined;
			}
			else if (lextable.declaredVariable() || lextable.declaredFunction())
			{
				throw ERROR_THROW_IN(123, lineNumber, -1);
			}

			break;

		case LEX_INTEGER_LITERAL:
			idTableIndex = idtable.getLiteralIndexByValue(atoi(name.c_str()));
			if (idTableIndex == TI_NULLIDX)
			{
				idTableIndex = idtable.addEntry({ "L" + std::to_string(lextable.size()), "", lextable.size(), type::i32, id_t::literal, name.c_str() });
			}
			token = LEX_LITERAL;
			break;

		case LEX_I32:
			dataType = type::i32;
			token = LEX_DATATYPE;
			break;

		case LEX_STR:
			dataType = type::str;
			token = LEX_DATATYPE;
			break;

		case LEX_STRING_LITERAL:
			idTableIndex = idtable.getLiteralIndexByValue(name.c_str());
			if (idTableIndex == TI_NULLIDX)
			{
				idTableIndex = idtable.addEntry({ "L" + std::to_string(lextable.size()), "", lextable.size(), type::str, id_t::literal, name.c_str() });
			}
			token = LEX_LITERAL;
			break;

		case LEX_OPENING_CURLY_BRACE:
			previousScope = currentScope;
			currentScope = lastFunctionName;
			break;

		case LEX_CLOSING_CURLY_BRACE:
			currentScope = previousScope;
			break;

		case LEX_OPENING_PARENTHESIS:
			previousScope = currentScope;
			currentScope = lastFunctionName;
			break;

		case LEX_CLOSING_PARENTHESIS:
			currentScope = previousScope;
			break;

		default:
			break;
		}

		lextable.addEntry(LexTable::Entry(token, lineNumber, idTableIndex));
	}

	if (!lextable.hasLexeme(LEX_MAIN))
		throw ERROR_THROW(130);

	log << "����������� ������ �������� ��� ������\n";
}