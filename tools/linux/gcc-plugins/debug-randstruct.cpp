/*
 * debug-randstruct gcc plugin could be use with the randomize_layout_plugin added with linux 5 to
 * retrieve coherent debug informations when structures are randomized.
 *
 * To compile this plugin :
 *
 * Usage:
 *
 */

#include <stdio.h>

#include <gcc-plugin.h>
#include <plugin-version.h>
#include <tree.h>
#include <print-tree.h>

#include "assert.h"

#define IS_ORIG_TYPE_NAME(node) (TYPE_NAME(TYPE_MAIN_VARIANT(node)) != NULL_TREE ? 1 : 0)
#define ORIG_TYPE_NAME(node) ((const unsigned char *)IDENTIFIER_POINTER(TYPE_NAME(TYPE_MAIN_VARIANT(node))))

int plugin_is_GPL_compatible = 1;

static struct plugin_info debug_randstruct_plugin_info = {
	.version	= "debug_randstruct_20190612",
	.help		= "no optional arguments\n",
};

static void start_line(unsigned int indentation)
{
	if (!indentation)
		printf("\r");
	else
		printf("\r%*c", indentation, ' ');
}

static long padding(long size, unsigned int indentation)
{
	long padded = 0;
	while (padded < size)
	{
		start_line(indentation);
		long delta = size - padded;
                if (delta > 64)
                {
	                printf("long long : 64;\n");
                        padded += 64;
                }
                else
                {
                        printf("long long : %ld;\n", delta);
                        padded += delta;
                }
	}
	return padded;
}

static void member(tree& member, tree& type, unsigned int indentation)
{
	assert(TREE_CODE(member) == TYPE_DECL | TREE_CODE(member) == FIELD_DECL);
        start_line(indentation);

	tree real_type = type;
	int is_ptr = 1;

	if (TREE_CODE(type) == POINTER_TYPE)
		real_type = TREE_TYPE(type);
	else
		is_ptr = 0;

        if (TREE_CODE(TYPE_NAME(real_type)) == IDENTIFIER_NODE)
                printf("%s", IDENTIFIER_POINTER(TYPE_NAME(real_type)));
        else if (TREE_CODE(TYPE_NAME(real_type)) == TYPE_DECL && DECL_NAME(TYPE_NAME(real_type)))
                printf("%s", IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(real_type))));
	else
		assert(0);

	if (is_ptr)
		printf("*");

	if (DECL_NAME(member));
      		printf(" %s", IDENTIFIER_POINTER(DECL_NAME(member)));

	if (DECL_BIT_FIELD_TYPE(member))
	{
		printf(" : %lu", tree_to_uhwi(DECL_SIZE(member)));
	}

	printf(";\n");

}

static unsigned long rewrite_structure(tree& decl, tree& type, unsigned int indentation)
{
	start_line(indentation);

	if (TREE_CODE(type) == RECORD_TYPE)
	        printf("struct");
	else if (TREE_CODE(type) == UNION_TYPE)
		printf("union");
	else
		assert(0);

	printf(" __attribute__((__packed__))");
	if (IS_ORIG_TYPE_NAME(type))
	        printf(" %s", ORIG_TYPE_NAME(type));
        printf(" {\n");

        tree field;
        long cum_offset = 0;

        for (field = TYPE_FIELDS(type); field != 0; field = TREE_CHAIN(field)) {
                if (field == NULL_TREE || field == error_mark_node)
                        continue;

               	unsigned long msize = tree_to_uhwi(TYPE_SIZE(TREE_TYPE(field)));
		if (TREE_CODE(type) != UNION_TYPE)
		{
			long offset = int_bit_position(field);
			cum_offset += padding(offset - cum_offset, indentation + 2);

                	assert(cum_offset == offset);
                	cum_offset += msize;
		}
		else if (msize > cum_offset)
			cum_offset = msize;

                tree mtype;
		if (DECL_BIT_FIELD_TYPE(field))
			mtype = DECL_BIT_FIELD_TYPE(field);
		else
			mtype = TREE_TYPE(field);

                if ((TREE_CODE(mtype) == RECORD_TYPE | TREE_CODE(mtype) == UNION_TYPE) & !IS_ORIG_TYPE_NAME(mtype))
                {
                        unsigned long check_msize = rewrite_structure(field, mtype, indentation + 2);
			assert(check_msize == msize);
                        continue;
                }

		member(field, mtype, indentation + 2);
        }

	unsigned long size = tree_to_uhwi(DECL_SIZE(decl));

	if (TREE_CODE(type) != UNION_TYPE)
		cum_offset += padding(size - cum_offset, indentation + 2);
	else if (size != cum_offset)
	{
		start_line(indentation + 2);
		printf("struct {\n");
		cum_offset = padding(size, indentation + 4);
		start_line(indentation + 2);
                printf("};\n");
	}
	assert(size == cum_offset);

	start_line(indentation);
	printf("}");
	if (DECL_NAME(decl))
		printf(" %s", IDENTIFIER_POINTER(DECL_NAME(decl)));

       	printf(";\n");
	if (!indentation)
		printf("\nint main() { return 0; }");

	return size;
}

static void debug_randstruct_finish_decl(void *event_data, void *data)
{
	tree decl = (tree)event_data;

	if (decl == NULL_TREE || decl == error_mark_node)
		return;

	tree type = TREE_TYPE(decl);

	if (TREE_CODE(decl) != VAR_DECL)
		return;

	if (TREE_CODE(type) != RECORD_TYPE && TREE_CODE(type) != UNION_TYPE)
		return;

	if (!IS_ORIG_TYPE_NAME(type))
		return;

	if (strcmp((char*) ORIG_TYPE_NAME(type), "task_struct"))
		return;

	rewrite_structure(decl, type, 0);
}

int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
	const char * const plugin_name = plugin_info->base_name;

	if (!plugin_default_version_check(version, &gcc_version)) {
		fprintf(stderr, "incompatible gcc/plugin versions");
		return 1;
	}

	register_callback(plugin_name, PLUGIN_INFO, NULL, &debug_randstruct_plugin_info);
	register_callback(plugin_name, PLUGIN_FINISH_DECL, debug_randstruct_finish_decl, NULL);

	return 0;
}
