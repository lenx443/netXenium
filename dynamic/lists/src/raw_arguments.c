#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "logs.h"
#include "raw_arguments.h"

int raw_args_push_argument(Raw_Arguments ras, char *argument) {
  Raw_Argument ra = {
      .ra_type = RAT_ARGUMENT,
      .ra_content =
          {
              .argument = strdup(argument),
          },
  };
  if (!ra.ra_content.argument) log_add_errno(NULL, ERROR, "Raw Argumets");
  if (!list_push_back(ras, &ra, sizeof(ra))) return 0;
  return 1;
}

int raw_args_push_property(Raw_Arguments ras, char *prop) {
  Raw_Argument ra = {
      .ra_type = RAT_PROPERTY,
      .ra_content =
          {
              .prop = strdup(prop),
          },
  };
  if (!ra.ra_content.prop) log_add_errno(NULL, ERROR, "Raw Argumets");
  if (!list_push_back(ras, &ra, sizeof(ra))) return 0;
  return 1;
}

int raw_args_push_concat(Raw_Arguments ras) {
  Raw_Argument ra = {
      .ra_type = RAT_CONCAT,
      .ra_content =
          {
              .no_content = 1,
          },
  };
  if (!list_push_back(ras, &ra, sizeof(ra))) return 0;
  return 1;
}

void raw_args_free(Raw_Arguments ras) {
  if (!list_valid(ras)) return;
  NODE_ptr ra_current = ras->head;
  while (ra_current != NULL) {
    NODE_ptr next = ra_current->next;
    Raw_Argument *ra = (Raw_Argument *)ra_current->point;
    if (ra->ra_type == RAT_ARGUMENT) free(ra->ra_content.argument);
    if (ra->ra_type == RAT_PROPERTY) free(ra->ra_content.prop);
    free(ra_current->point);
    free(ra_current);
    ra_current = next;
  }
}
