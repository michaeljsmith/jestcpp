#include <stdio.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <vector>
#include <cctype>
#include <cstdarg>

namespace jest
{
	using boost::shared_ptr;
	using boost::static_pointer_cast;
	using boost::variant;
	using std::vector;
	using std::string;

	namespace types
	{
		shared_ptr<void const> type(new int);
	}

	namespace types
	{
		shared_ptr<void const> symbol(new int);
	}

	struct typed_value
	{
		shared_ptr<void const> type;
		shared_ptr<void const> value;
	};

	namespace types
	{
		shared_ptr<void const> typed_value(new int);
	}

	struct typed_cell
	{
		shared_ptr<typed_value const> head;
		shared_ptr<typed_cell const> tail;
	};

	namespace types
	{
		shared_ptr<void const> typed_cell(new int);
	}

	struct pattern
	{
		shared_ptr<void const> type;
		shared_ptr<void const> value;
	};

	namespace pattern_types
	{
		shared_ptr<void const> symbol(new int);
		shared_ptr<void const> variable(new int);
	}

	struct pattern_cell
	{
		shared_ptr<pattern const> head;
		shared_ptr<pattern const> tail;
	};

	namespace pattern_types
	{
		shared_ptr<void const> cell(new int);
	}

	namespace patterns
	{
		struct context
		{
		};

		class fatal_error {};

		void fatal(context* c, char const* format, ...)
		{
			va_list args;
			va_start(args, format);
			vfprintf(stderr, format, args);
			fputs("\n", stderr);
			throw fatal_error();
		}

		bool equal(context* c,
			shared_ptr<void const> const& type0,
			shared_ptr<void const> const& value0,
			shared_ptr<void const> const& type1,
			shared_ptr<void const> const& value1)
		{
			if (type0 != type1)
				return false;

			if (type0 == types::type)
			{
				// TODO: parameterized types.
				return value0 == value1;
			}
			else if (type0 == types::symbol)
			{
				return value0 == value1;
			}
			else if (type0 == types::typed_cell)
			{
				shared_ptr<typed_cell const> cell0 = static_pointer_cast<
					typed_cell const>(value0);
				shared_ptr<typed_cell const> cell1 = static_pointer_cast<
					typed_cell const>(value1);

				return
					equal(c,
						cell0->head->type, cell0->head->value,
						cell1->head->type, cell1->head->value) &&
					equal(c,
						types::typed_cell, cell0->tail,
						types::typed_cell, cell1->tail);
			}
			else if (type0 == types::typed_value)
			{
				shared_ptr<typed_value const> val0 = static_pointer_cast<
					typed_value const>(value0);
				shared_ptr<typed_value const> val1 = static_pointer_cast<
					typed_value const>(value1);

				return equal(c,
					val0->type, val0->value,
					val1->type, val1->type);
			}
			else
			{
				fatal(c, "equal() cannot compare values of unrecognized types.");
				return false;
			}
		}

		struct binding
		{
			binding(std::string symbol, shared_ptr<void const> type,
				shared_ptr<void const> value)
				: symbol(symbol), type(type), value(value) {}
			std::string symbol;
			shared_ptr<void const> type;
			shared_ptr<void const> value;
		};

		struct match_result
		{
			vector<shared_ptr<binding const> > bindings;
		};

		shared_ptr<match_result const> match(
				context* c,
				shared_ptr<void const> pattern_type, shared_ptr<void const> pattern_value,
				shared_ptr<void const> type, shared_ptr<void const> value)
		{
			if (pattern_type == pattern_types::symbol)
			{
				if (type != types::symbol)
					return shared_ptr<match_result const>();
				shared_ptr<string const> pattern_symbol =
					static_pointer_cast<string const>(pattern_value);
				shared_ptr<string const> symbol =
					static_pointer_cast<string const>(value);
				if (pattern_symbol != symbol)
					return shared_ptr<match_result const>();
				shared_ptr<match_result> result(new match_result);
				return result;
			}
			else if (pattern_type == pattern_types::variable)
			{
				shared_ptr<string const> pattern_symbol =
					static_pointer_cast<string const>(pattern_value);
				shared_ptr<patterns::binding> binding(new patterns::binding(
							*pattern_symbol, type, value));
				shared_ptr<match_result> result(new match_result);
				result->bindings.push_back(binding);
				return result;
			}
			else if (pattern_type == pattern_types::cell)
			{
				if (type != types::typed_cell)
					return shared_ptr<match_result const>();
				shared_ptr<jest::pattern_cell const> pattern_cell =
					static_pointer_cast<jest::pattern_cell const>(pattern_value);
				shared_ptr<typed_cell const> cell =
					static_pointer_cast<typed_cell const>(value);
				shared_ptr<match_result const> head_result = match(c,
						pattern_cell->head->type, pattern_cell->head->value,
						cell->head->type, cell->head->value);
				if (!head_result)
					return head_result;
				shared_ptr<match_result const> tail_result = match(c,
						pattern_cell->tail->type, pattern_cell->tail->value,
						types::typed_cell, cell->tail);
				if (!tail_result)
					return tail_result;

				// Combine bindings.
				shared_ptr<match_result> result(new match_result);
				result->bindings = head_result->bindings;
				for (int j = 0, jcnt = int(tail_result->bindings.size());
					j < jcnt; ++j)
				{
					shared_ptr<binding const> tail_binding = tail_result->bindings[j];

					shared_ptr<binding const> matching_binding;
					for (int i = 0, icnt = int(head_result->bindings.size()); i < icnt; ++i)
					{
						shared_ptr<binding const> head_binding = head_result->bindings[i];

						if (head_binding->symbol == tail_binding->symbol)
							matching_binding = head_binding;
					}

					if (matching_binding)
					{
						if (!equal(c, tail_binding->type, tail_binding->value,
									tail_binding->type, tail_binding->value))
							return shared_ptr<match_result const>();
					}
					else
					{
						result->bindings.push_back(tail_binding);
					}
				}

				return result;
			}
			else
			{
				fatal(c, "INTERNAL ERROR: Invalid pattern type.");
				return shared_ptr<match_result const>();
			}
		}
	}

#define parse_assert(c, cond) \
	do \
	{ \
		if (!cond) \
		{ \
			fatal((c), "internal parsing error: %s", #cond); \
		} \
	} while (0)

	namespace parsing
	{
		class fatal_error {};

		typedef std::string identifier;

		struct form;

		typedef variant<
			shared_ptr<parsing::identifier const>,
			shared_ptr<parsing::form const> > expression;

		struct parameter
		{
			parameter(shared_ptr<identifier const> const& name,
					shared_ptr<expression const> const& type)
				: name(name), type(type) {}

			shared_ptr<identifier const> name;
			shared_ptr<expression const> type;
		};

		struct prototype
		{
			prototype(shared_ptr<expression const> const& name,
					vector<shared_ptr<parameter const> > const& parameters)
				: name(name), parameters(parameters) {}
			shared_ptr<expression const> name;
			vector<shared_ptr<parameter const> > parameters;
		};

		typedef variant<
			shared_ptr<parsing::identifier const>,
			shared_ptr<parsing::prototype const> > target;

		struct define;

		typedef variant<
			shared_ptr<parsing::define const>,
			shared_ptr<parsing::expression const> > statement;

		struct form
		{
			form(shared_ptr<expression const> const& head,
					std::vector<shared_ptr<parsing::statement const> > const& statements)
				: head(head), statements(statements) {}
			shared_ptr<expression const> head;
			std::vector<shared_ptr<parsing::statement const> > statements;
		};

		struct define
		{
			define(shared_ptr<parsing::target const> const& target,
					shared_ptr<parsing::expression const> const& expression)
				: target(target), expression(expression) {}
			shared_ptr<parsing::target const> target;
			shared_ptr<parsing::expression const> expression;
		};

		struct module
		{
			module(std::vector<shared_ptr<parsing::define const> > const& defines)
				: defines(defines) {}
			std::vector<shared_ptr<parsing::define const> > defines;
		};

		enum symbol
		{
			eof,
			ident,
			lt,
			gt,
			colon,
			equal
		};
		struct context
		{
			FILE* f;
			parsing::symbol symbol;
			string token;
			char ch;
		};

		shared_ptr<module const> parse_module(context* c);
		shared_ptr<define const> parse_define(context* c);
		shared_ptr<statement const> parse_statement(context* c);
		shared_ptr<statement const> parse_symbol_statement(context* c);
		shared_ptr<define const> parse_symbol_define(context* c);
		shared_ptr<statement const> parse_form_statement(context* c);
		shared_ptr<statement const> parse_form_statement_contents(context* c);
		struct form_statement_tail
		{
			form_statement_tail(vector<shared_ptr<parameter const> > const& parameters,
			vector<shared_ptr<statement const> > const& statements,
			shared_ptr<parsing::expression const> const& expression)
				: parameters(parameters), statements(statements), expression(expression) {}
			vector<shared_ptr<parameter const> > parameters;
			vector<shared_ptr<statement const> > statements;
			shared_ptr<parsing::expression const> expression;
		};
		shared_ptr<form_statement_tail const> parse_form_statement_tail(context* c);
		struct prototype_non_thunk_tail
		{
			prototype_non_thunk_tail(shared_ptr<parsing::expression const> const&
					initial_type, vector<shared_ptr<parameter const> > const&
					subsequent_parameters, shared_ptr<parsing::expression const> const&
					expression)
				: initial_type(initial_type), subsequent_parameters(subsequent_parameters),
				expression(expression) {}
			shared_ptr<parsing::expression const> initial_type;
			vector<shared_ptr<parameter const> > subsequent_parameters;
			shared_ptr<parsing::expression const> expression;
		};
		struct form_expression_non_thunk_tail
		{
			form_expression_non_thunk_tail(vector<shared_ptr<statement const> > const&
					subsequent_arguments)
				: subsequent_arguments(subsequent_arguments) {}
			vector<shared_ptr<statement const> > subsequent_arguments;
		};
		typedef variant<
			shared_ptr<prototype_non_thunk_tail const>,
			shared_ptr<form_expression_non_thunk_tail const> > non_thunk_tail;
		shared_ptr<non_thunk_tail const> parse_non_thunk_tail(context* c);
		shared_ptr<expression const> parse_define_tail(context* c);
		struct form_expression_tail
		{
			form_expression_tail(vector<shared_ptr<statement const> > const& statements)
				: statements(statements) {}
			vector<shared_ptr<statement const> > statements;
		};
		shared_ptr<form_expression_tail const> parse_form_expression_tail(context* c);
		shared_ptr<define const> parse_form_define(context* c);
		shared_ptr<parameter const> parse_parameter(context* c);
		shared_ptr<expression const> parse_expression(context* c);
		shared_ptr<expression const> parse_form_expression(context* c);

		void error(context* c, char const* format, ...)
		{
			va_list args;
			va_start(args, format);
			vfprintf(stderr, format, args);
			fputs("\n", stderr);
		}

		void fatal(context* c, char const* format, ...)
		{
			va_list args;
			va_start(args, format);
			vfprintf(stderr, format, args);
			fputs("\n", stderr);
			throw fatal_error();
		}

		void read_symbol(context* c)
		{
			while (std::isspace(c->ch))
				c->ch = fgetc(c->f);

			if (feof(c->f))
			{
				c->symbol = eof;
				c->token = "";
			}
			else if (c->ch == '<')
			{
				c->symbol = lt;
				c->token = "<";
				c->ch = fgetc(c->f);
			}
			else if (c->ch == '>')
			{
				c->symbol = gt;
				c->token = ">";
				c->ch = fgetc(c->f);
			}
			else if (c->ch == ':')
			{
				c->symbol = colon;
				c->token = ":";
				c->ch = fgetc(c->f);
			}
			else if (c->ch == '=')
			{
				c->symbol = equal;
				c->token = "=";
				c->ch = fgetc(c->f);
			}
			else if (std::isalnum(c->ch) || c->ch == '_')
			{
				c->symbol = ident;
				c->token = "";
				while (std::isalnum(c->ch) || c->ch == '_')
				{
					c->token.push_back(c->ch);
					c->ch = fgetc(c->f);
				}
			}
			else
			{
				fatal(c, "invalid character \'%c\'.", c->ch);
				c->ch = fgetc(c->f);
			}
		}

		bool accept(context* c, symbol symbol)
		{
			if (c->symbol == symbol)
			{
				read_symbol(c);
				return true;
			}
			return false;
		}

		void expect(context* c, symbol symbol)
		{
			if (c->symbol != symbol)
				fatal(c, "unexpected: %s", c->token.c_str());
			read_symbol(c);
		}

		shared_ptr<identifier const> parse_identifier(context* c)
		{
			if (c->symbol == ident)
			{
				shared_ptr<identifier> identifier(new string(c->token));
				read_symbol(c);
				return identifier;
			}
			return shared_ptr<identifier const>();
		}

		//module = {define}
		shared_ptr<module const> parse_module(context* c)
		{
			std::vector<shared_ptr<parsing::define const> > defines;
			while (c->symbol != eof)
			{
				shared_ptr<define const> define = parse_define(c);
				parse_assert(c, define);
				defines.push_back(define);
			}
			shared_ptr<module> module(new parsing::module(defines));

			return module;
		}

		//define =
		//	form_define
		//	| symbol_define
		shared_ptr<define const> parse_define(context* c)
		{
			shared_ptr<define const> define;
			define = (!define ? parse_form_define(c) : define);
			define = (!define ? parse_symbol_define(c) : define);
			return define;
		}

		//statement =
		//	form_statement
		//	| symbol_statement
		shared_ptr<statement const> parse_statement(context* c)
		{
			shared_ptr<statement const> statement;
			statement = (!statement ? parse_form_statement(c) : statement);
			statement = (!statement ? parse_symbol_statement(c) : statement);
			return statement;
		}

		//symbol_statement =
		//	identifier ["=" expression]
		shared_ptr<statement const> parse_symbol_statement(context* c)
		{
			shared_ptr<parsing::identifier const> identifier = parse_identifier(c);
			if (!identifier)
				return shared_ptr<parsing::statement const>();
			if (accept(c, equal))
			{
				shared_ptr<parsing::expression const> expression = parse_expression(c);
				shared_ptr<parsing::target> target(new parsing::target(identifier));
				shared_ptr<parsing::define> define(new parsing::define(
						target, expression));
				shared_ptr<parsing::statement> statement(new parsing::statement(define));
				return statement;
			}
			else
			{
				shared_ptr<parsing::expression> expression(new parsing::expression(
						identifier));
				shared_ptr<parsing::statement> statement(new parsing::statement(
							expression));
				return statement;
			}
		}

		//symbol_define =
		//	identifier "=" expression
		shared_ptr<define const> parse_symbol_define(context* c)
		{
			shared_ptr<identifier const> identifier = parse_identifier(c);
			if (!identifier)
				return shared_ptr<define const>();
			expect(c, equal);
			shared_ptr<expression const> expression = parse_expression(c);
			shared_ptr<parsing::target> target(new parsing::target(identifier));
			shared_ptr<parsing::define> define(new parsing::define(
					target, expression));
			return define;
		}

		//form_statement =
		//	"<" form_statement_contents
		shared_ptr<statement const> parse_form_statement(context* c)
		{
			if (!accept(c, lt))
				return shared_ptr<statement const>();
			return parse_form_statement_contents(c);
		}

		//form_statement_contents =
		//	identifier form_statement_tail
		//	| {statement} ">"
		shared_ptr<statement const> parse_form_statement_contents(context* c)
		{
			shared_ptr<identifier const> identifier = parse_identifier(c);
			if (identifier)
			{
				shared_ptr<form_statement_tail const> tail =
					parse_form_statement_tail(c);

				parse_assert(c, tail);
				if (!tail->parameters.empty())
				{
					parse_assert(c, tail->statements.empty());

					shared_ptr<parsing::expression> name(new parsing::expression(
								identifier));
					shared_ptr<parsing::prototype> prototype(new parsing::prototype(
							name, tail->parameters));
					shared_ptr<parsing::target> target(new parsing::target(prototype));
					shared_ptr<parsing::define> define(new parsing::define(target,
								tail->expression));
					shared_ptr<parsing::statement> statement(new parsing::statement(
								define));
					return statement;
				}
				else if (!tail->statements.empty())
				{
					shared_ptr<parsing::expression> head(new parsing::expression(
								identifier));
					shared_ptr<parsing::form> form(new parsing::form(head,
								tail->statements));
					shared_ptr<parsing::expression> expression(new parsing::expression(
								form));
					shared_ptr<parsing::statement> statement(new parsing::statement(
								expression));
					return statement;
				}
				else
				{
					parse_assert(c, 0);
					return shared_ptr<statement const>();
				}
			}
			else
			{
				shared_ptr<expression const> head = parse_expression(c);
				parse_assert(c, head);
				vector<shared_ptr<statement const> > statements;
				while (c->symbol != gt)
				{
					shared_ptr<statement const> statement = parse_statement(c);
					parse_assert(c, statement);
					statements.push_back(statement);
				}

				expect(c, gt);

				shared_ptr<parsing::form> form(new parsing::form(head, statements));
				shared_ptr<parsing::expression> expression(new parsing::expression(
							form));
				shared_ptr<parsing::statement> statement(new parsing::statement(
							expression));

				return statement;
			}
		}

		//form_statement_tail =
		//	">" [define_tail]
		//	| identifier non_thunk_tail
		//	| {statement} ">"
		shared_ptr<form_statement_tail const> parse_form_statement_tail(context* c)
		{
			vector<shared_ptr<parameter const> > parameters;
			vector<shared_ptr<statement const> > statements;
			shared_ptr<expression const> expression;

			if (c->symbol == gt)
			{
				expression = parse_define_tail(c);
			}
			else
			{
				shared_ptr<identifier const> identifier = parse_identifier(c);
				if (identifier)
				{
					shared_ptr<non_thunk_tail const> tail = parse_non_thunk_tail(c);
					if (shared_ptr<prototype_non_thunk_tail const> const* ptail =
							boost::get<shared_ptr<prototype_non_thunk_tail const> >(tail.get()))
					{
						shared_ptr<parameter> initial_parameter(new parameter(
								identifier, (*ptail)->initial_type));
						parameters.push_back(initial_parameter);
						for (int i = 0, cnt = int((*ptail)->subsequent_parameters.size());
								i < cnt; ++i)
							parameters.push_back((*ptail)->subsequent_parameters[i]);
						expression = (*ptail)->expression;
					}
					else if (shared_ptr<form_expression_non_thunk_tail const> const* ftail =
							boost::get<shared_ptr<form_expression_non_thunk_tail const> >(
								tail.get()))
					{
						shared_ptr<parsing::expression> expression(new parsing::expression(
									identifier));
						shared_ptr<parsing::statement> statement(new parsing::statement(
									expression));
						statements.push_back(statement);
						for (int i = 0, cnt = int((*ftail)->subsequent_arguments.size());
								i < cnt; ++i)
							statements.push_back((*ftail)->subsequent_arguments[i]);
					}
					else
					{
						parse_assert(c, 0);
					}
				}
				else
				{
					while (c->symbol != gt)
					{
						shared_ptr<statement const> statement = parse_statement(c);
						parse_assert(c, statement);
						statements.push_back(statement);
					}
					expect(c, gt);
				}
			}

			shared_ptr<form_statement_tail> tail(new form_statement_tail(
					parameters, statements, expression));
			return tail;
		}

		//non_thunk_tail =
		//	":" expression {parameter} ">" define_tail
		//	| {statement} ">"
		shared_ptr<non_thunk_tail const> parse_non_thunk_tail(context* c)
		{
			if (accept(c, colon))
			{
				shared_ptr<expression const> initial_type = parse_expression(c);
				vector<shared_ptr<parameter const> > subsequent_parameters;
				while (c->symbol != gt)
				{
					shared_ptr<parameter const> parameter = parse_parameter(c);
					parse_assert(c, parameter);
					subsequent_parameters.push_back(parameter);
				}
				expect(c, gt);
				shared_ptr<expression const> expression = parse_define_tail(c);
				shared_ptr<prototype_non_thunk_tail> ptail(new prototype_non_thunk_tail(
						initial_type, subsequent_parameters, expression));
				shared_ptr<non_thunk_tail> tail(new non_thunk_tail(ptail));
				return tail;
			}
			else
			{
				vector<shared_ptr<statement const> > subsequent_arguments;
				while (c->symbol != gt)
				{
					shared_ptr<statement const> statement = parse_statement(c);
					parse_assert(c, statement);
					subsequent_arguments.push_back(statement);
				}
				expect(c, gt);
				shared_ptr<form_expression_non_thunk_tail> ftail(new
					form_expression_non_thunk_tail(subsequent_arguments));
				shared_ptr<non_thunk_tail> tail(new non_thunk_tail(ftail));
				return tail;
			}
		}

		//define_tail =
		//	"=" expression
		shared_ptr<expression const> parse_define_tail(context* c)
		{
			if (!accept(c, equal))
				return shared_ptr<expression const>();
			shared_ptr<expression const> expression = parse_expression(c);
			parse_assert(c, expression);
			return expression;
		}

		//form_expression_tail =
		//	{statement} ">"
		shared_ptr<form_expression_tail const> parse_form_expression_tail(context* c)
		{
			vector<shared_ptr<statement const> > statements;
			while (c->symbol != gt)
			{
				shared_ptr<parsing::statement const> statement = parse_statement(c);
				parse_assert(c, statement);
				statements.push_back(statement);
			}
			expect(c, gt);
			shared_ptr<form_expression_tail> tail(new form_expression_tail(statements));
			return tail;
		}

		//form_define =
		//	"<" expression {parameter} ">" = expression
		shared_ptr<define const> parse_form_define(context* c)
		{
			if (!accept(c, lt))
				return shared_ptr<define const>();

			shared_ptr<expression const> name = parse_expression(c);
			parse_assert(c, name);
			vector<shared_ptr<parameter const> > parameters;
			while (c->symbol != gt)
			{
				shared_ptr<parameter const> parameter = parse_parameter(c);
				parse_assert(c, parameter);
				parameters.push_back(parameter);
			}
			shared_ptr<parsing::prototype> prototype(new parsing::prototype(name,
						parameters));

			expect(c, gt);
			expect(c, equal);

			shared_ptr<parsing::expression const> expression = parse_expression(c);
			shared_ptr<parsing::target> target(new parsing::target(prototype));
			shared_ptr<parsing::define> define(new parsing::define(target,
					expression));
			return define;
		}

		//parameter =
		//	identifier ":" identifier
		shared_ptr<parameter const> parse_parameter(context* c)
		{
			shared_ptr<identifier const> name = parse_identifier(c);
			parse_assert(c, name);

			expect(c, colon);

			shared_ptr<expression const> type = parse_expression(c);
			parse_assert(c, type);
			shared_ptr<parsing::parameter> parameter(new parsing::parameter(name, type));
			return parameter;
		}

		//expression =
		//	identifier
		//	| form_expression
		shared_ptr<expression const> parse_expression(context* c)
		{
			shared_ptr<parsing::identifier const> identifier = parse_identifier(c);
			if (identifier)
			{
				shared_ptr<parsing::expression> expression(new parsing::expression(
						identifier));
				return expression;
			}
			return parse_form_expression(c);
		}

		//form_expression =
		//	"<" expression form_expression_tail
		shared_ptr<expression const> parse_form_expression(context* c)
		{
			if (!accept(c, lt))
				return shared_ptr<expression const>();
			shared_ptr<expression const> head = parse_expression(c);
			parse_assert(c, head);
			shared_ptr<form_expression_tail const> tail = parse_form_expression_tail(c);
			parse_assert(c, tail);
			shared_ptr<parsing::form> form(new parsing::form(head, tail->statements));
			shared_ptr<parsing::expression> expression(new parsing::expression(
						form));
			return expression;
		}

		shared_ptr<parsing::module const> parse_file(char const* filename)
		{
			context c;
			c.f = fopen(filename, "r");
			if (!c.f)
			{
				char buf[1024];
				perror(buf);
				fatal(&c, "error opening file \"%s\": %s", filename, buf);
			}

			c.ch = fgetc(c.f);
			read_symbol(&c);

			return parse_module(&c);
		}
	}
}

int main(int /*argc*/, char* /*argv*/[])
{
	try
	{
		boost::shared_ptr<jest::parsing::module const> module_ast =
			jest::parsing::parse_file("test/test.jest");
	}
	catch (jest::parsing::fatal_error /*e*/)
	{
		fprintf(stderr, "Unrecoverable error; exitting.\n");
		return 1;
	}
}
