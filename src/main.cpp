#include <stdio.h>

namespace jest
{
	namespace detail
	{
		template <typename T> void* tag_of() {static int x; return &x;}
	}

	namespace types
	{
		namespace detail {struct symbol_tag {};} void* symbol = jest::detail::tag_of<detail::symbol_tag>();
	}

	struct typed_value
	{
		void* type;
		void* value;
	};

	namespace types
	{
		namespace detail {struct typed_value_tag {};} void* typed_value = jest::detail::tag_of<detail::typed_value_tag>();
	}

	struct typed_cell
	{
		typed_value* head;
		typed_cell* tail;
	};

	namespace types
	{
		namespace detail {struct typed_cell_tag {};} void* typed_cell = jest::detail::tag_of<detail::typed_cell_tag>();
	}

	struct pattern
	{
		void* type;
		void* value;
	};

	namespace pattern_types
	{
		namespace detail {struct symbol_tag {};} void* symbol = jest::detail::tag_of<detail::symbol_tag>();
	}

	struct pattern_cell
	{
		pattern* head;
		pattern_cell* tail;
	};

	namespace pattern_types
	{
		namespace detail {struct cell_tag {};} void* cell = jest::detail::tag_of<detail::cell_tag>();
	}

	namespace parsing
	{
		//module = {define}
		struct module
		{
			std::vector<boost::shared_ptr<define const> > defines;
		};

		//define =
		//	symbol_define
		//	| form_define
		struct statement
		{
			boost::variant<
				boost::shared_ptr<symbol_define>,
				boost::shared_ptr<form_define> >
		};

		//statement =
		//	symbol_statement
		//	| form_statement
		struct statement
		{
			boost::variant<
				boost::shared_ptr<symbol_statement>,
				boost::shared_ptr<form_statement> >
		};

		//symbol_statement =
		//	symbol_define
		//	| symbol_expression

		//symbol_define =
		//	ident "=" expression

		//symbol_expression =
		//	ident

		//form_statement =
		//	"<" ident form_statement_tail

		//form_statement_tail =
		//	form_define_tail
		//	| form_expression_tail

		//form_define_tail =
		//	prototype_tail "=" expression

		//prototype_tail =
		//	{parameter} ">"

		//form_expression_tail =
		//	{statement} ">"

		//form_define =
		//	prototype "=" expression

		//prototype =
		//	"<" ident {parameter} ">"

		//parameter =
		//	ident ":" ident

		//expression =
		//	form_expression
		//	| symbol_expression

		//form_expression =
		//	"<" ident form_expression_tail
	}
}

int main(int /*argc*/, char* /*argv*/[])
{
	return 0;
}

