#include <stdio>

namespace jest
{
	namespace detail
	{
		template <typename T> void* tag_of() {static int x; return &x;}
	}

	namespace types
	{
		namespace detail {struct symbol_tag {};} void* symbol = detail::tag_of<detail::symbol_tag>();
	}

	struct typed_value
	{
		void* type;
		void* value;
	};

	namespace types
	{
		namespace detail {struct typed_value_tag {};} void* typed_value = detail::tag_of<detail::typed_value_tag>();
	}

	struct typed_cell
	{
		typed_value* head;
		typed_cell* tail;
	};

	namespace types
	{
		namespace detail {struct typed_cell_tag {};} void* typed_cell = detail::tag_of<detail::typed_cell_tag>();
	}

	struct pattern
	{
		void* type;
		void* value;
	};

	namespace pattern_types
	{
		namespace detail {struct symbol_tag {};} void* symbol = detail::tag_of<detail::symbol_tag>();
	}

	struct pattern_cell
	{
		pattern* head;
		pattern_cell* tail;
	};

	namespace pattern_types
	{
		namespace detail {struct cell_tag {};} void* cell = detail::tag_of<detail::cell_tag>();
	}
}

int main(int /*argc*/, char* /*argv*/[])
{
	return 0;
}

