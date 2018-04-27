// sendEmail.cpp - functions to send an email using curl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "sendEmail.h"
#include "logMsg.h"

// free alloced components of t_EmailData struct
static void freeEmailData(t_EmailData * data)
{
    if (data->email)
    {
        free((void *)data->email);
        data->email = NULL;
    }
}

static t_EmailData * initEmailData(t_EmailData * data, const char * to, const char * from, 
    const char * subject, const char * msg)
{
    static const int maxMsgSize = 1024;

    time_t tloc = time(NULL);
    struct tm * timeinfo = localtime(&tloc);

    data->len = 0;
    data->pos = 0;
    data->email = (char *)malloc(maxMsgSize);  // max size of resulting message

    if (data->email != NULL)
    {
        memset((void *)(data->email), 0, maxMsgSize);

        char * p = data->email;
        int i = 0;
        i += sprintf(p + i, "MIME-Version: 1.0\r\n");
        i += strftime(p + i, 48, "Date: %a, %d %b %Y %T %z\r\n", timeinfo);
        i += sprintf(p + i, "To: %s\r\n", to);
        i += sprintf(p + i, "From: %s\r\n", from);
        i += sprintf(p + i, "Subject: %s\r\n", subject);
        i += sprintf(p + i, "Content-Type: multipart/alternative; boundary=border\r\n");
        i += sprintf(p + i, "\r\n");
        i += sprintf(p + i, "--border\r\n");
        i += sprintf(p + i, "Content-Type: text/plain; charset=UTF-8\r\n");
        i += sprintf(p + i, "\r\n");
        i += sprintf(p + i, "%s", msg);
        i += sprintf(p + i, "\r\n");
        i += sprintf(p + i, "--border\r\n");
        i += sprintf(p + i, "Content-Type: text/html; charset=UTF-8\r\n");
        i += sprintf(p + i, "\r\n");
        i += sprintf(p + i, "<div dir=\"ltr\">%s</div>\r\n", msg);
        i += sprintf(p + i, "\r\n");
        i += sprintf(p + i, "--border--\r\n");

        if (i >= maxMsgSize)
        {
            logMsg("initEmailData exceeded max message size! len=%d, max=%d\n", i, maxMsgSize);
        }
        data->len = i;
    }
    return data;
}

// function used to read source data for sending via curl-post
static size_t readCallback(char * ptr, size_t size, size_t nmemb, void * userp)
{
    if (size == 0 || nmemb == 0 || (size * nmemb) < 1) 
        return 0;

    t_EmailData * upload = (t_EmailData *)userp;
    const char * pEmail = upload->email + upload->pos;  // advance pointer past what has been sent

    if (upload->pos < upload->len && pEmail)
    {
        size_t len = strlen(pEmail);
        if (len > size * nmemb) 
            return CURL_READFUNC_ABORT;
        memcpy(ptr, pEmail, len);       // copy remaining msg to ptr
        upload->pos += len;
        return len;
    }
    return 0;
}

// send email via curl
bool sendEmail(const char * from, const char * to, const char * passwd, const char * subj, const char * mesg)
{
    bool success = true;
    CURL * curl = curl_easy_init();

    if (!curl)
    {
        fprintf(stderr, "sendTextAlert curl open failed\n");
        return false;  // unable to get curl handle
    }

    struct curl_slist * recipients = NULL;
    recipients = curl_slist_append(recipients, to);

    t_EmailData data = {0};                      // init struct with zeros
    initEmailData(&data, to, from, subj, mesg);  // fill in real data

    if (data.email != NULL)
    {
        // set username/passwd/mail_server
        curl_easy_setopt(curl, CURLOPT_USERNAME, from);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, passwd);
        curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587/");
        // start with a normal connection, upgrade to TLS using STARTTLS
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, readCallback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &data);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        // useful for debugging encryped traffic
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        //printf("html email:\n%s\n", data.email);

        // send the message
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            fprintf(stderr, "sendTextAlert curl failed: %s\n", curl_easy_strerror(res));
            success = false;
        }
        freeEmailData(&data);
    }
    else
    {
        success = false;
    }
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
    return success;
}
