/*
 *
 * 2010 (c) by Christian Klippel
 *
 * licensed under GNU GPL
 *
 */

#ifndef AIC14_H
#define AIC14_H

int aic14_open();
int aic14_close(void);
int aic14_readall(void);
int aic14_set8k(void);
int aic14_set16k(void);
int aic14_enable_mic(void);

#endif
