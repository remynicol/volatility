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

#include "assert.h"

#define ORIG_TYPE_NAME(node) \
	(TYPE_NAME(TYPE_MAIN_VARIANT(node)) != NULL_TREE ? ((const unsigned char *)IDENTIFIER_POINTER(TYPE_NAME(TYPE_MAIN_VARIANT(node)))) : (const unsigned char *)"anonymous")

int plugin_is_GPL_compatible = 1;

static struct plugin_info debug_randstruct_plugin_info = {
	.version	= "debug_randstruct_20190612",
	.help		= "no optional arguments\n",
};

static void rewrite_structure(tree& decl, bool inner_struct = false)
{
	tree type = TREE_TYPE(decl);

	//FILE* fp = fopen("poipoi.txt", "w");
        //fclose(fp);

        printf("\rstruct __attribute__((__packed__)) %s {\n", ORIG_TYPE_NAME(type));

        tree field;
        long cum_offset = 0;

        for (field = TYPE_FIELDS(type); field != 0; field = TREE_CHAIN(field)) {
                tree mtype = TREE_TYPE(field);

                //if (!DECL_NAME(field))
                        //continue;

                unsigned long size = tree_to_uhwi(DECL_SIZE(field));
                long offset = int_bit_position(field);

                for (; cum_offset < offset; cum_offset += 8)
                        printf("  char : 1;\n");

                assert(cum_offset == offset);
                cum_offset += size;

                assert(TREE_CODE(field) == TYPE_DECL | TREE_CODE(field) == FIELD_DECL);

                if (TREE_CODE(TYPE_NAME(mtype)) == IDENTIFIER_NODE)
                        printf("  %s ", IDENTIFIER_POINTER(TYPE_NAME(mtype)));
                else if (TREE_CODE(TYPE_NAME(mtype)) == TYPE_DECL && DECL_NAME(TYPE_NAME(mtype)))
                        printf("  %s ", IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(mtype))));

		if (DECL_NAME(field))
                	printf("%s;", IDENTIFIER_POINTER(DECL_NAME(field)));

		printf("\n");
        }

	printf("} obj_%s;\n\nvoid main() { }", ORIG_TYPE_NAME(type));
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

	if (strcmp((char*) ORIG_TYPE_NAME(type), "task_struct"))
		return;

	rewrite_structure(decl);
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
