#ifndef _LOGGER_H
#define _LOGGER_H

void logger_init(char *filepath);
void logger_log_client_data(int client_id, int tot_time, int que_time, int n_products, int que_changes);
void logger_log_general_data();
void logger_cleanup();

#endif
