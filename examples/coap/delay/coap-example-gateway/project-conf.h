#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

// Set the logging level to see your INFO and WARN messages in the Cooja output
#define LOG_LEVEL_APP LOG_LEVEL_INFO

// Set the maximum CoAP message chunk size
#define REST_MAX_CHUNK_SIZE 64



// CRITICAL FOR PHASE 1: Enable floating point support for printf/logging.
// Since SRTT, RTTVAR, and the Threshold use decimal values and mathematical 
// functions, this ensures your LOG_INFO statements print the numbers correctly 
// instead of throwing errors or printing blanks.
#define PRINTF_FORMAT_FLOATING_POINT 1

#endif /* PROJECT_CONF_H_ */