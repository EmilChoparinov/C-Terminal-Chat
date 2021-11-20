#ifndef _SQL_INJECT_CHECKER_H_
#define _SQL_INJECT_CHECKER_H_

/**
 * @brief check if an sql query is safe or not
 *
 * @param query query to check safety
 * @return int 0 if OK, 0 if not
 */
int sic_is_sql_ok(char *query);

#endif