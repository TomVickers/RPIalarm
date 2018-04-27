// sendEmail.h - functions to send an email using curl

#pragma once

#include <stdio.h>

typedef struct {
    char * email;
    size_t len;
    size_t pos;
} t_EmailData;

bool sendEmail(const char * from, const char * to, const char * passwd, const char * subj, const char * mesg);
