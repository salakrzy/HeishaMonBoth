/*
  Copyright (C) CurlyMo

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/


#ifndef _RULES_H_
#define _RULES_H_

#include <stdint.h>

#ifndef ESP32
  #define F
  #define MEMPOOL_SIZE 16000
  typedef struct pbuf {
    struct pbuf *next;
    void *payload;
    uint16_t tot_len;
    uint16_t len;
    uint8_t type;
    uint8_t flags;
    uint16_t ref;
  } pbuf;
#else
  #include <Arduino.h>
  #include "lwip/pbuf.h"
  #define MEMPOOL_SIZE MMU_SEC_HEAP_SIZE
#endif

#define EPSILON  0.000001

/*
 * max(sizeof(vm_vfloat_t), sizeof(vm_vinteger_t), sizeof(vm_vnull_t))
 */
#define MAX_VARSTACK_NODE_SIZE 7

#define MAX(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

typedef enum {
  TOPERATOR = 1,
  TFUNCTION = 2,
  TSTRING = 3,
  TNUMBER = 4,
  TNUMBER1 = 5,
  TNUMBER2 = 6,
  TNUMBER3 = 7,
  TEOF = 8,
  LPAREN = 9,
  RPAREN = 10,
  TCOMMA = 11,
  TIF = 12,
  TELSE = 13,
  TTHEN = 14,
  TEVENT = 15,
  TCEVENT = 16,
  TEND = 17,
  TVAR = 18,
  TASSIGN = 19,
  TSEMICOLON = 20,
  TTRUE = 21,
  TFALSE = 22,
  TSTART = 23,
  VCHAR = 24,
  VINTEGER = 25,
  VFLOAT = 26,
  VNULL = 27,
} token_types;

typedef struct rules_t {
  unsigned short nr;

  struct {
#if defined(DEBUG) or defined(ESP32)
  #ifdef ESP32
      unsigned long first;
      unsigned long second;
  #else
      struct timespec first;
      struct timespec second;
  #endif
#endif
  }  timestamp;

  struct {
    int parsed;
    int vars;
  } pos;

  /* Continue here after we processed
   * another rule call.
   */
  struct {
    uint16_t go;
    uint16_t ret;
  } cont;

  /* To which rule do we return after
   * being called from another rule.
   */
  int caller;

  struct {
    unsigned char *buffer;
    unsigned int nrbytes;
    unsigned int bufsize;
  } ast;

  struct {
    unsigned char *buffer;
    unsigned int nrbytes;
    unsigned int bufsize;
  } varstack;

  void *userdata;
} rules_t;

typedef struct rule_options_t {
  /*
   * Identifying callbacks
   */
  int (*is_token_cb)(char *text, unsigned int *pos, unsigned int size);
  int (*is_event_cb)(char *text, unsigned int *pos, unsigned int size);

  /*
   * Variables
   */
  unsigned char *(*get_token_val_cb)(struct rules_t *obj, uint16_t token);
  void (*cpy_token_val_cb)(struct rules_t *obj, uint16_t token);
  void (*clr_token_val_cb)(struct rules_t *obj, uint16_t token);
  void (*set_token_val_cb)(struct rules_t *obj, uint16_t token, uint16_t val);
  void (*prt_token_val_cb)(struct rules_t *obj, char *out, int size);

  /*
   * Events
   */
  int (*event_cb)(struct rules_t *obj, char *name);
} rule_options_t;

extern struct rule_options_t rule_options;

/*
 * Each position field is the closest
 * aligned width of 11 bits.
 */
#define VM_GENERIC_FIELDS \
  uint8_t type; \
  uint16_t ret;

typedef struct vm_vchar_t {
  VM_GENERIC_FIELDS
  char value[];
} __attribute__((packed)) vm_vchar_t;

typedef struct vm_vnull_t {
  VM_GENERIC_FIELDS
} __attribute__((packed)) vm_vnull_t;

typedef struct vm_vinteger_t {
  VM_GENERIC_FIELDS
  int value;
} __attribute__((packed)) vm_vinteger_t;

typedef struct vm_vfloat_t {
  VM_GENERIC_FIELDS
  float value;
} __attribute__((packed)) vm_vfloat_t;

typedef struct vm_tgeneric_t {
  VM_GENERIC_FIELDS
} __attribute__((packed)) vm_tgeneric_t;

typedef struct vm_tstart_t {
  VM_GENERIC_FIELDS
  uint16_t go;
} __attribute__((packed)) vm_tstart_t;

typedef struct vm_tif_t {
  VM_GENERIC_FIELDS
  uint16_t go;
  uint16_t true_;
  uint16_t false_;
} __attribute__((packed)) vm_tif_t;

typedef struct vm_lparen_t {
  VM_GENERIC_FIELDS
  uint16_t go;
  uint16_t value;
} __attribute__((packed)) vm_lparen_t;

typedef struct vm_tnumber_t {
  VM_GENERIC_FIELDS
  uint8_t token[];
} __attribute__((packed)) vm_tnumber_t;

typedef struct vm_ttrue_t {
  VM_GENERIC_FIELDS
  uint8_t nrgo;
  uint16_t go[];
} __attribute__((packed)) vm_ttrue_t;

typedef struct vm_tfunction_t {
  VM_GENERIC_FIELDS
  uint16_t token;
  uint16_t value;
  uint8_t nrgo;
  uint16_t go[];
} __attribute__((packed)) vm_tfunction_t;

typedef struct vm_tvar_t {
  VM_GENERIC_FIELDS
  uint16_t go;
  uint16_t value;
  uint8_t token[];
} __attribute__((packed)) vm_tvar_t;

typedef struct vm_tevent_t {
  VM_GENERIC_FIELDS
  uint16_t go;
  uint8_t token[];
} __attribute__((packed)) vm_tevent_t;

typedef struct vm_tcevent_t {
  VM_GENERIC_FIELDS
  uint8_t token[];
} __attribute__((packed)) vm_tcevent_t;

typedef struct vm_toperator_t {
  VM_GENERIC_FIELDS
  uint8_t token;
  uint16_t left;
  uint16_t right;
  uint16_t value;
} __attribute__((packed)) vm_toperator_t;

typedef struct vm_teof_t {
  uint8_t type;
} __attribute__((packed)) vm_teof_t;

unsigned int alignedvarstack(int v);
int rule_initialize(struct pbuf *input, struct rules_t ***rules, int *nrrules, struct pbuf *mempool, void *userdata);
void rules_gc(struct rules_t ***obj, unsigned int nrrules);
int rule_run(struct rules_t *obj, int validate);
void valprint(struct rules_t *obj, char *out, int size);

#endif
