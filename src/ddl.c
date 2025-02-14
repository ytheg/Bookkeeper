#include <stddef.h>
#include <stdio.h>
#include "ddl.h"


char	*file_names[] = {
	"ACCOUNTS",
	"TRANSACTIONS",
	"JOURNAL",
	"CURRENCIES",
	"EXCHANGERATES",
	NULL
	};


char	*element_names[] = {
	"ACCOUNT_ID",
	"PARENT_ID",
	"ACCOUNT_NUMBER",
	"ACCOUNT_NAME",
	"ACCOUNT_TYPE",
	"DESCRIPTION",
	"CREATED_AT",
	"UPDATED_AT",
	"TRANSACTION_ID",
	"TRANSACTION_DATE",
	"REFERENCE_NUMBER",
	"ENTRY_ID",
	"AMOUNT",
	"ENTRY_TYPE",
	"BASE_CURRENCY_ID",
	"CURRENCY_ID",
	"CURRENCY_CODE",
	"CURRENCY_SYMBOL",
	"CURRENCY_NAME",
	"DECIMAL_PLACES",
	"EXCHANGE_RATE",
	"RATE_ID",
	"EFFECTIVE_DATE",
	NULL
	};


int	accounts_f[] = {
	ACCOUNT_ID,
	PARENT_ID,
	CURRENCY_ID,
	ACCOUNT_NUMBER,
	ACCOUNT_NAME,
	ACCOUNT_TYPE,
	DESCRIPTION,
	CREATED_AT,
	UPDATED_AT,
	-1
	};


int	transactions_f[] = {
	TRANSACTION_ID,
	BASE_CURRENCY_ID,
	TRANSACTION_DATE,
	DESCRIPTION,
	REFERENCE_NUMBER,
	CREATED_AT,
	UPDATED_AT,
	-1
	};


int	journal_f[] = {
	ENTRY_ID,
	TRANSACTION_ID,
	ACCOUNT_ID,
	CURRENCY_ID,
	AMOUNT,
	EXCHANGE_RATE,
	ENTRY_TYPE,
	CREATED_AT,
	-1
	};


int	currencies_f[] = {
	CURRENCY_ID,
	CURRENCY_CODE,
	CURRENCY_SYMBOL,
	CURRENCY_NAME,
	DECIMAL_PLACES,
	-1
	};


int	exchangerates_f[] = {
	RATE_ID,
	CURRENCY_ID,
	BASE_CURRENCY_ID,
	EXCHANGE_RATE,
	EFFECTIVE_DATE,
	-1
	};


int	*file_table[] = {
	accounts_f,
	transactions_f,
	journal_f,
	currencies_f,
	exchangerates_f,
	NULL
	};


int	accounts_x0[] = {
	ACCOUNT_ID,
	-1
	};


int	*accounts_i[] = {
	accounts_x0,
	NULL
	};


int	transactions_x0[] = {
	TRANSACTION_ID,
	-1
	};


int	*transactions_i[] = {
	transactions_x0,
	NULL
	};


int	journal_x0[] = {
	ENTRY_ID,
	-1
	};


int	*journal_i[] = {
	journal_x0,
	NULL
	};


int	currencies_x0[] = {
	CURRENCY_ID,
	-1
	};


int	*currencies_i[] = {
	currencies_x0,
	NULL
	};


int	*exchangerates_i[] = {
	currencies_x0,
	NULL
	};


int	**index_table[] = {
	accounts_i,
	transactions_i,
	journal_i,
	currencies_i,
	exchangerates_i,
	NULL
	};


