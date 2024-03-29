#include "pch.h"
#include "LexicalAnalyzer.h"
#include "FST.h"

TTM::LexicalAnalyzer::LexicalAnalyzer(LexTable& lextable, IdTable& idtable)
	: lextable(lextable), idtable(idtable)
{	}

char TTM::LexicalAnalyzer::tokenize(const std::string& str)
{
	FST::FST fst[] = {
		FST_I32, FST_STR, FST_FN, FST_IF, FST_ELSE, FST_LET,
		FST_RET, FST_ECHO, FST_MAIN,
		FST_OPENING_PARENTHESIS, FST_CLOSING_PARENTHESIS, FST_SEMICOLON, FST_COMMA,
		FST_OPENING_CURLY_BRACE, FST_CLOSING_CURLY_BRACE,
		FST_PLUS, FST_MINUS, FST_ASTERISK, FST_SLASH, FST_PERCENT,
		FST_ASSIGN,
		FST_ID, FST_STRING_LITERAL, FST_INTEGER_LITERAL
	};
	const int size = sizeof(fst) / sizeof(fst[0]);
	const char tokens[] = {
		LEX_I32, LEX_STR, LEX_FN, LEX_IF, LEX_ELSE, LEX_LET,
		LEX_RET, LEX_ECHO, LEX_MAIN,
		LEX_OPENING_PARENTHESIS, LEX_CLOSING_PARENTHESIS, LEX_SEMICOLON, LEX_COMMA,
		LEX_OPENING_CURLY_BRACE, LEX_CLOSING_CURLY_BRACE,
		LEX_PLUS, LEX_MINUS, LEX_ASTERISK, LEX_SLASH, LEX_PERCENT,
		LEX_ASSIGN,
		LEX_ID, LEX_STRING_LITERAL, LEX_INTEGER_LITERAL
	};

	for (int i = 0; i < size; ++i) {
		if (execute(str, fst[i]))
		{
			return tokens[i];
		}
	}

	return EOF;
}

void TTM::LexicalAnalyzer::includeStdlibFunctions()
{
	idtable.addEntry({ "parseInt", "", TI_NULLIDX, it::data_type::i32, it::id_type::function, "0" });
	idtable.addEntry({ "s", "parseInt", TI_NULLIDX, it::data_type::str, it::id_type::parameter, "0" });

	idtable.addEntry({ "concat", "", TI_NULLIDX, it::data_type::str, it::id_type::function, "0" });
	idtable.addEntry({ "a", "concat", TI_NULLIDX, it::data_type::str, it::id_type::parameter, "0" });
	idtable.addEntry({ "b", "concat", TI_NULLIDX, it::data_type::str, it::id_type::parameter, "0" });
}

void TTM::LexicalAnalyzer::Scan(const std::vector<std::pair<std::string, int>>& sourceCode, Logger& log)
{
	using type = it::data_type;
	using id_t = it::id_type;

	std::string currentScope = "";
	std::string previousScope = currentScope;
	std::string lastFunctionName = "";

	int literalsCounter = 0;
	id_t idType = id_t::unknown;
	type dataType = type::undefined;

	bool unaryMinusCorrection = false;

	includeStdlibFunctions();

	for (size_t i = 0; i < sourceCode.size(); ++i)
	{
		const auto& [name, lineNumber] = sourceCode[i];
		char token = tokenize(name);
		if (token == EOF)
		{
			throw ERROR_THROW_LEX(129, lineNumber);
		}

		int idTableIndex = TI_NULLIDX;

		switch (token)
		{
		case LEX_MAIN:
			idTableIndex = idtable.getIdIndexByName("", name);
			if (idTableIndex == TI_NULLIDX)
			{
				if (!lextable.declaredFunction())
					throw ERROR_THROW_LEX(120, lineNumber);
				if (dataType != type::i32)
					throw ERROR_THROW_LEX(121, lineNumber);

				idTableIndex = idtable.addEntry({ name, "", lextable.size(), dataType, id_t::function, "0" });
				idType = id_t::unknown;
				dataType = type::undefined;
				lastFunctionName = name;
			}
			else if (lextable.declaredFunction())
			{
				throw ERROR_THROW_LEX(131, lineNumber);
			}
			break;

		case LEX_ID:
			if (i < sourceCode.size() - 1 && tokenize(sourceCode[i + 1].first) == LEX_OPENING_PARENTHESIS)
			{
				idTableIndex = idtable.getIdIndexByName("", name);
			}
			else
			{
				idTableIndex = idtable.getIdIndexByName(currentScope, name);
			}
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
				else if (lextable.declaredDatatype())
				{
					idType = id_t::parameter;
				}
				else
				{
					throw ERROR_THROW_LEX(124, lineNumber);
				}

				if (idType == id_t::unknown)
					throw ERROR_THROW_LEX(120, lineNumber);
				if (dataType == type::undefined)
					throw ERROR_THROW_LEX(121, lineNumber);

				idTableIndex = idtable.addEntry({ name, currentScope, lextable.size(), dataType, idType, "0" });
				idType = id_t::unknown;
				dataType = type::undefined;
			}
			else if (lextable.declaredVariable() || lextable.declaredFunction() || lextable.declaredDatatype())
			{
				throw ERROR_THROW_LEX(123, lineNumber);
			}
			else if (name == currentScope)
			{
				throw ERROR_THROW_LEX(125, lineNumber);
			}
			break;

		case LEX_I32:
			dataType = type::i32;
			token = LEX_DATATYPE;
			break;

		case LEX_STR:
			dataType = type::str;
			token = LEX_DATATYPE;
			break;

		case LEX_INTEGER_LITERAL:
			idTableIndex = idtable.getLiteralIndexByValue(atoi(name.c_str()));
			if (idTableIndex == TI_NULLIDX)
			{
				idTableIndex = idtable.addEntry({ "L" + std::to_string(literalsCounter), "", lextable.size(), type::i32, id_t::literal, name.c_str() });
				++literalsCounter;
			}
			token = LEX_LITERAL;
			break;

		case LEX_STRING_LITERAL:
			if (name == "''")
				throw ERROR_THROW_LEX(126, lineNumber);

			idTableIndex = idtable.getLiteralIndexByValue(name.c_str());
			if (idTableIndex == TI_NULLIDX)
			{
				idTableIndex = idtable.addEntry({ "L" + std::to_string(literalsCounter), "", lextable.size(), type::str, id_t::literal, name.c_str() });
				++literalsCounter;
			}
			token = LEX_LITERAL;
			break;

		case LEX_OPENING_CURLY_BRACE:
		case LEX_OPENING_PARENTHESIS:
			previousScope = currentScope;
			currentScope = lastFunctionName;
			break;

		case LEX_CLOSING_CURLY_BRACE:
		case LEX_CLOSING_PARENTHESIS:
			currentScope = previousScope;
			break;

		case LEX_MINUS:
			if (lextable[lextable.size() - 1].lexeme == LEX_ASSIGN
				|| lextable[lextable.size() - 1].lexeme == LEX_OPENING_PARENTHESIS
				|| lextable[lextable.size() - 1].lexeme == LEX_RET)
			{
				lextable.addEntry({ LEX_OPENING_PARENTHESIS, lineNumber, TI_NULLIDX });
				int tmp = idtable.getLiteralIndexByValue(0);
				if (tmp == TI_NULLIDX)
				{
					tmp = idtable.addEntry({ "L" + std::to_string(literalsCounter), "", lextable.size(), type::i32, id_t::literal, "0" });
					++literalsCounter;
				}
				lextable.addEntry({ LEX_LITERAL, lineNumber, tmp });
				unaryMinusCorrection = true;
			}
			break;

		default:
			break;
		}

		lextable.addEntry({ token, lineNumber, idTableIndex });

		if ((token == LEX_LITERAL || (token == LEX_ID
			&& idtable[lextable[lextable.size() - 1].idTableIndex].idType != id_t::function)) && unaryMinusCorrection)
		{
			lextable.addEntry({ LEX_CLOSING_PARENTHESIS, lineNumber, TI_NULLIDX });
			unaryMinusCorrection = false;
		}
	}

	if (!lextable.hasLexeme(LEX_MAIN))
		throw ERROR_THROW(130);

	if (lextable.lexemeCount(LEX_OPENING_PARENTHESIS) != lextable.lexemeCount(LEX_CLOSING_PARENTHESIS)
		|| lextable.lexemeCount(LEX_OPENING_CURLY_BRACE) != lextable.lexemeCount(LEX_CLOSING_CURLY_BRACE))
		throw ERROR_THROW(122);

	log << "����������� ������ �������� ��� ������\n";
}