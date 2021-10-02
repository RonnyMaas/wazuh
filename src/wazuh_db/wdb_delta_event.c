#include "wdb.h"

sqlite3_stmt * wdb_get_cache_stmt(wdb_t * wdb, char const *query) {
    sqlite3_stmt * ret_val = NULL;
    if (NULL != wdb && NULL != query) {
        struct stmt_cache_list *node_stmt = NULL;
        for (node_stmt = wdb->cache_list; node_stmt ; node_stmt=node_stmt->next) {
            if (node_stmt->value.query) {
                if (strcmp(node_stmt->value.query, query) == 0)
                {
                    if (sqlite3_reset(node_stmt->value.stmt) != SQLITE_OK || sqlite3_clear_bindings(node_stmt->value.stmt) != SQLITE_OK) {
                        mdebug1("DB(%s) sqlite3_reset() stmt(%s): %s", wdb->id, sqlite3_sql(node_stmt->value.stmt), sqlite3_errmsg(wdb->db));
                    }
                    ret_val = node_stmt->value.stmt;
                    break;
                }
            }
        }
        bool is_first_element = true;
        if (NULL == ret_val) {
            struct stmt_cache_list *new_item = NULL;
            if (NULL == wdb->cache_list) {
                os_malloc(sizeof(struct stmt_cache_list), wdb->cache_list);
                new_item = wdb->cache_list;
            } else {
                node_stmt = wdb->cache_list;
                while (node_stmt->next){
                    node_stmt = node_stmt->next;
                }
                is_first_element = false;
                os_malloc(sizeof(struct stmt_cache_list), node_stmt->next);
                //Add element in the end list
                new_item = node_stmt->next;
            }
            new_item->next = NULL;
            os_malloc(strlen(query) + 1, new_item->value.query);
            strcpy(new_item->value.query, query);

            if (sqlite3_prepare_v2(wdb->db, new_item->value.query, -1, &new_item->value.stmt, NULL) != SQLITE_OK) {
                merror("DB(%s) sqlite3_prepare_v2() : %s", wdb->id, sqlite3_errmsg(wdb->db));
                os_free(new_item->value.query);
                if (is_first_element) {
                    os_free(wdb->cache_list);
                    wdb->cache_list = NULL;
                } else {
                    os_free(node_stmt->next);
                    node_stmt->next = NULL;
                }
            } else {
                ret_val = new_item->value.stmt;
            }
        }
    }
    return ret_val;
}

bool wdb_single_row_insert_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data) {
    bool ret_val = false;
    if (NULL != kv_value) {
        char query[OS_SIZE_2048] = { 0 };
        strcat(query, "DELETE FROM ");
        strcat(query, kv_value->value);
        strcat(query, ";");
        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);

        if (NULL != stmt) {
            ret_val = SQLITE_DONE == wdb_step(stmt) ? true : false;
        } else {
            mdebug1("Cannot get cache statement");
        }
        ret_val = ret_val && wdb_insert_dbsync(wdb, kv_value, data);
    }
    return ret_val;
}

bool wdb_insert_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data) {
    bool ret_val = false;

    if (NULL != data && NULL != wdb && NULL != kv_value) {
        char query[OS_SIZE_2048] = { 0 };
        strcat(query, "INSERT INTO ");
        strcat(query, kv_value->value);
        strcat(query, " VALUES (");
        struct column_list const *column = NULL;

        for (column = kv_value->column_list; column ; column=column->next) {
            strcat(query, "?");
            if (column->next) {
                strcat(query, ",");
            }
        }
        strcat(query, ");");

        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);

        if (NULL != stmt) {
            char * data_temp = NULL;
            os_strdup(data, data_temp);

            char *field_value = strtok(data_temp, FIELD_SEPARATOR_DBSYNC);
            for (column = kv_value->column_list; column ; column=column->next) {
                if (column->value.is_old_implementation) {
                    if (FIELD_TEXT == column->value.type) {
                        if (SQLITE_OK != sqlite3_bind_text(stmt, column->value.index, "", -1, NULL)) {
                            merror("DB(%s) sqlite3_bind_text(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                        }
                    } else {
                        if (SQLITE_OK != sqlite3_bind_int(stmt, column->value.index, 0)) {
                            merror("DB(%s) sqlite3_bind_int(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                        }
                    }
                } else {
                    if (NULL != field_value) {
                        if (FIELD_TEXT == column->value.type) {
                            if (SQLITE_OK != sqlite3_bind_text(stmt, column->value.index, strcmp(field_value, "NULL") == 0 ? "" : field_value, -1, NULL)) {
                                merror("DB(%s) sqlite3_bind_text(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        } else {
                            if (SQLITE_OK != sqlite3_bind_int(stmt, column->value.index, strcmp(field_value, "NULL") == 0 ? 0 : atoi(field_value))) {
                                merror("DB(%s) sqlite3_bind_int(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        }
                        if (column->next) {
                            field_value = strtok(NULL, FIELD_SEPARATOR_DBSYNC);
                        }
                    }
                }
            }
            ret_val = SQLITE_DONE == wdb_step(stmt);
            os_free(data_temp);
        } else {
            mdebug1("Cannot get cache statement");
        }
    }
    return ret_val;
}

bool wdb_modify_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data)
{
    bool ret_val = false;
    if (NULL != data && NULL != wdb && NULL != kv_value) {
        char query[OS_SIZE_2048] = { 0 };
        strcat(query, "UPDATE ");
        strcat(query, kv_value->value);
        strcat(query, " SET ");

        const size_t size = sizeof(char*) * (os_strcnt(data, '|') + 1);
        char ** field_values = NULL;
        os_calloc(1, size, field_values);
        char **curr = field_values;

        char * data_temp = NULL;
        os_strdup(data, data_temp);
        char *tok = strtok(data_temp, FIELD_SEPARATOR_DBSYNC);

        while (NULL != tok) {
            *curr = tok;
            tok = strtok(NULL, FIELD_SEPARATOR_DBSYNC);
            ++curr;
        }

        if (curr) {
            *curr = NULL;
        }

        bool first_condition_element = true;
        curr = field_values;
        struct column_list const *column = NULL;
        for (column = kv_value->column_list; column ; column=column->next) {
            if (!column->value.is_old_implementation && curr && NULL != *curr) {
                if (!column->value.is_pk && strcmp(*curr, "NULL") != 0) {
                    if (first_condition_element) {
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                        first_condition_element = false;
                    } else {
                        strcat(query, ",");
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                    }
                }
                ++curr;
            }
        }
        strcat(query, " WHERE ");

        first_condition_element = true;
        for (column = kv_value->column_list; column ; column=column->next) {
            if (column->value.is_pk) {
                if (first_condition_element) {
                    strcat(query, column->value.name);
                    strcat(query, "=?");
                    first_condition_element = false;
                } else {
                    strcat(query, " AND ");
                    strcat(query, column->value.name);
                    strcat(query, "=?");
                }
            }
        }
        strcat(query, ";");

        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);

        if (NULL != stmt) {
            int index = 1;

            curr = field_values;
            for (column = kv_value->column_list; column ; column=column->next) {
                if (!column->value.is_old_implementation && curr && NULL != *curr) {
                    if (!column->value.is_pk && strcmp(*curr, "NULL") != 0) {
                        if (FIELD_TEXT == column->value.type) {
                            if (SQLITE_OK != sqlite3_bind_text(stmt, index, *curr, -1, NULL)) {
                                merror("DB(%s) sqlite3_bind_text(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        } else {
                            if (SQLITE_OK != sqlite3_bind_int(stmt, index, atoi(*curr))) {
                                merror("DB(%s) sqlite3_bind_int(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        }
                        ++index;
                    }
                    ++curr;
                }
            }

            curr = field_values;
            for (column = kv_value->column_list; column ; column=column->next) {
                if (!column->value.is_old_implementation && curr && NULL != *curr) {
                    if (column->value.is_pk && strcmp(*curr, "NULL") != 0) {
                        if (FIELD_TEXT == column->value.type) {
                            if (SQLITE_OK != sqlite3_bind_text(stmt, index, *curr, -1, NULL)) {
                                merror("DB(%s) sqlite3_bind_text(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        } else {
                            if (SQLITE_OK != sqlite3_bind_int(stmt, index, atoi(*curr))) {
                                merror("DB(%s) sqlite3_bind_int(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                            }
                        }
                        ++index;
                    }
                    ++curr;
                }
            }
            ret_val = SQLITE_DONE == wdb_step(stmt);
        } else {
            mdebug1("Cannot get cache statement");
        }
        os_free(field_values);
        os_free(data_temp);
    }
    return ret_val;
}

bool wdb_delete_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data)
{
    bool ret_val = false;
    if (NULL != wdb && NULL != data) {
        char query[OS_SIZE_2048] = { 0 };
        strcat(query, "DELETE FROM ");
        strcat(query, kv_value->value);
        strcat(query, " WHERE ");

        bool first_condition_element = true;
        struct column_list const *column = NULL;
        for (column = kv_value->column_list; column ; column=column->next) {
            if (!column->value.is_old_implementation) {
                if (column->value.is_pk) {
                    if (first_condition_element) {
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                        first_condition_element = false;
                    } else {
                        strcat(query, " AND ");
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                    }
                }
            }
        }
        strcat(query, ";");

        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);

        if (NULL != stmt) {
            char *data_temp = NULL;
            os_strdup(data, data_temp);
            char *field_value = strtok(data_temp, FIELD_SEPARATOR_DBSYNC);

            struct column_list const *column = NULL;
            int index = 1;
            for (column = kv_value->column_list; column ; column=column->next) {
                if (!column->value.is_old_implementation) {
                    if (NULL != field_value) {
                        if (column->value.is_pk) {
                            if (FIELD_TEXT == column->value.type) {
                                if (SQLITE_OK != sqlite3_bind_text(stmt, index, strcmp(field_value, "NULL") == 0 ? "" : field_value, -1, NULL)) {
                                    merror("DB(%s) sqlite3_bind_text(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                                }
                            } else {
                                if (SQLITE_OK != sqlite3_bind_int(stmt, index, strcmp(field_value, "NULL") == 0 ? 0 : atoi(field_value))) {
                                    merror("DB(%s) sqlite3_bind_int(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                                }
                            }
                            ++index;
                        }
                        if (column->next) {
                            field_value = strtok(NULL, FIELD_SEPARATOR_DBSYNC);
                        }
                    }
                }
            }
            ret_val = SQLITE_DONE == wdb_step(stmt);
            os_free(data_temp);
        } else {
            mdebug1("Cannot get cache statement");
        }
    }
    return ret_val;
}


void wdb_select_dbsync(wdb_t * wdb, struct kv const *kv_value, const char *data, char *output)
{
    if (NULL != wdb && NULL != data) {
        char query[OS_SIZE_2048] = { 0 };
        bool first_condition_element = true;
        struct column_list const *column = NULL;

        strcat(query, "SELECT ");
        for (column = kv_value->column_list; column ; column=column->next) {
            if (!column->value.is_old_implementation) {
                if (first_condition_element) {
                    first_condition_element = false;
                } else {
                    strcat(query, ", ");
                }
                strcat(query, column->value.name);
            }
        }
        strcat(query, " FROM ");
        strcat(query, kv_value->value);
        strcat(query, " WHERE ");

        first_condition_element = true;
        for (column = kv_value->column_list; column ; column=column->next) {
            if (!column->value.is_old_implementation) {
                if (column->value.is_pk) {
                    if (first_condition_element) {
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                        first_condition_element = false;
                    } else {
                        strcat(query, " AND ");
                        strcat(query, column->value.name);
                        strcat(query, "=?");
                    }
                }
            }
        }
        strcat(query, ";");
        sqlite3_stmt *stmt = wdb_get_cache_stmt(wdb, query);
        if (NULL != stmt) {
            char * data_temp = NULL;
            os_strdup(data, data_temp);
            char *field_value = strtok(data_temp, FIELD_SEPARATOR_DBSYNC);

            struct column_list const *column = NULL;
            int index = 1;
            for (column = kv_value->column_list; column ; column=column->next) {
                if (!column->value.is_old_implementation) {
                    if (NULL != field_value) {
                        if (column->value.is_pk) {
                            if (FIELD_TEXT == column->value.type) {
                                if (SQLITE_OK != sqlite3_bind_text(stmt, index, strcmp(field_value, "NULL") == 0 ? "" : field_value, -1, NULL)) {
                                    merror("DB(%s) sqlite3_bind_text(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                                }
                            } else {
                                if (SQLITE_OK != sqlite3_bind_int(stmt, index, strcmp(field_value, "NULL") == 0 ? 0 : atoi(field_value))) {
                                    merror("DB(%s) sqlite3_bind_int(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                                }
                            }
                            ++index;
                        }
                        if (column->next) {
                            field_value = strtok(NULL, FIELD_SEPARATOR_DBSYNC);
                        }
                    }
                }
            }
            index = 0;
            int len = strlen(output);
            switch (sqlite3_step(stmt)) {
                case SQLITE_ROW:
                    for (column = kv_value->column_list; column ; column=column->next) {
                        if (!column->value.is_old_implementation) {
                            if (FIELD_TEXT == column->value.type) {
                                len += snprintf(output + len, OS_SIZE_6144 - len - WDB_RESPONSE_OK_SIZE - 1, "%s", (char *)sqlite3_column_text(stmt, index));
                            }
                            else if (FIELD_INTEGER == column->value.type) {
                                len += snprintf(output + len, OS_SIZE_6144 - len - WDB_RESPONSE_OK_SIZE - 1, "%d", sqlite3_column_int(stmt, index));
                            }
                            else if (FIELD_REAL == column->value.type) {
                                len += snprintf(output + len, OS_SIZE_6144 - len - WDB_RESPONSE_OK_SIZE - 1, "%f", sqlite3_column_double(stmt, index));
                            }
                            len += snprintf(output + len, OS_SIZE_6144 - len - WDB_RESPONSE_OK_SIZE - 1, FIELD_SEPARATOR_DBSYNC);
                            ++index;
                        }
                    }
                    break;
                case SQLITE_DONE:
                    break;
                default:
                    merror("DB(%s) sqlite3_step(): %s", wdb->id, sqlite3_errmsg(wdb->db));
                    break;
            }
            os_free(data_temp);
        } else {
            mdebug1("Cannot get cache statement");
        }
    }
}

