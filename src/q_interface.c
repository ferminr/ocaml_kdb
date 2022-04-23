/*
 * Copyright (c) 2022 Fermin Reig
 *
 * q_interface.c
 */

// Uncomment next line to disable assertions
// #define NDEBUG

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/custom.h>
#include <caml/fail.h>
#include <caml/bigarray.h>
#include "q_interface.h"

// forward declarations

static value q_to_ocaml(const K q_val);
static K ocaml_to_q(const value v);


///////////////////////////////////////////////////
// Functions to convert kdb+ values to OCaml values
///////////////////////////////////////////////////

static value mk_caml_value(const int tag, value v) {
  CAMLparam1(v);
  CAMLlocal1(result);

  result = caml_alloc(1, tag);
  Store_field(result, 0, v);
  CAMLreturn(result);
}

static value mk_caml_value_two(const int tag, value v, value attrib) {
  CAMLparam2(v, attrib);
  CAMLlocal1(result);

  assert(Is_block(v));
  assert(Is_long(attrib));

  result = caml_alloc(2, tag);
  Store_field(result, 0, v);
  Store_field(result, 1, attrib);
  CAMLreturn(result);
}


static value mk_caml_dict(const K q_val) {
  CAMLparam0 ();
  CAMLlocal1 (result);

  result = caml_alloc(3, 0);
  K keys = kK(q_val)[0];
  K values = kK(q_val)[1];
  Store_field(result, 0, q_to_ocaml(keys));
  Store_field(result, 1, q_to_ocaml(values));
  Store_field(result, 2, Val_int(q_val->u)); // Atribute
  CAMLreturn (mk_caml_value(tag_dict, result));
  }

static value mk_caml_table(const K q_val) {
  CAMLparam0 ();
  CAMLlocal1 (tbl);

  tbl = caml_alloc(3, 0);
  Store_field(tbl, 0, q_to_ocaml(kK(q_val->k)[0]));
  Store_field(tbl, 1, q_to_ocaml(kK(q_val->k)[1]));
  Store_field(tbl, 2, Val_int(q_val->u)); // Attribute
  CAMLreturn (mk_caml_value(tag_table, tbl));
}

static value mk_caml_byte_array(const int caml_tag, const K q_val) {
  CAMLparam0 ();
  CAMLlocal2 (attrib, arr);

  long dims[1];
  dims[0] = q_val->n;
  attrib = Val_int(q_val->u);
  arr = caml_ba_alloc(CAML_BA_UINT8 | CAML_BA_C_LAYOUT, 1, kG(q_val), dims);
  CAMLreturn (mk_caml_value_two(caml_tag, arr, attrib));
}

static value mk_caml_scalar_array(const int caml_tag, const int arr_ty, void * data, const K q_val) {
  CAMLparam0 ();
  CAMLlocal2 (attrib, arr);

  long dims[1];
  dims[0] = q_val->n;
  attrib = Val_int(q_val->u);
  arr = caml_ba_alloc(arr_ty | CAML_BA_C_LAYOUT, 1, data, dims);
  CAMLreturn (mk_caml_value_two(caml_tag, arr, attrib));
}


// See also mk_caml_string_array_helper
static value mk_caml_array(const K q_val)
{
  // Note: this is largely the same as the funcion caml_alloc_array in the caml
  // RTS (alloc.c)
  CAMLparam0 ();
  CAMLlocal2 (v, result);

  const unsigned long size = q_val->n;

  if (0 == size) {
    CAMLreturn (Atom(0));
  } else {
    result = caml_alloc(size, 0);
    K *q_elems = kK(q_val);
    unsigned long i;
    for (i = 0; i < size; i++) {
      /* The two statements below must be separate because of evaluation
         order (don't take the address &Field(result, i) before
         calling q_to_ocaml, which may cause a GC and move result). */
      v = q_to_ocaml(q_elems[i]);
      caml_modify(&Field(result, i), v);
    }
    CAMLreturn (result);
  }
}

// See also mk_caml_array
static value mk_caml_string_array_helper(const K q_val) {
  CAMLparam0 ();
  CAMLlocal2 (v, result);

  assert(q_val->t > 0);

  const unsigned long size = q_val->n;
  if(0 == size) {
    CAMLreturn(Atom(0));
  } else {
    result = caml_alloc(size, 0);
    char **q_arr = kS(q_val);
    unsigned long i;
    for (i = 0; i < size ; i++) {
      // The two statements below must be separate because of evaluation
      // order (don't take the address &Field(result, i) before
      // calling caml_copy_string, which may cause a GC and move result).
      v = caml_copy_string(q_arr[i]);
      caml_modify(&Field(result, i), v);
    }
    CAMLreturn(result);
  }
}

static value mk_caml_string_array(const K q_val) {
  CAMLparam0 ();
  CAMLlocal2 (attrib, arr);

  attrib = Val_int(q_val->u);
  arr = mk_caml_string_array_helper(q_val);
  CAMLreturn (mk_caml_value_two(tag_v_symbol, arr, attrib));
}


static int tag_for_scalar(const int ty) {
  switch(ty){
  case (q_int32):  return tag_int32;
  case (q_month):  return tag_month;
  case (q_date):   return tag_date;
  case (q_minute): return tag_minute;
  case (q_second): return tag_second;
  case (q_time):   return tag_time;
  default: {
    caml_failwith("tag_for_scalar: impossible tag\n");
  }
  }
}

static int tag_for_scalar64(const int ty) {
  switch(ty){
  case (q_int64):     return tag_int64;
  case (q_timestamp): return tag_timestamp;
  case (q_timespan):  return tag_timespan;
  default: {
    caml_failwith("tag_for_scalar64: impossible tag\n");
  }
  }
}

static int tag_for_vector(const int ty) {
  switch(ty){
  case (-q_bool):      return tag_v_bool;
  case (-q_byte):      return tag_v_byte;
  case (-q_int16):     return tag_v_int16;
  case (-q_int32):     return tag_v_int32;
  case (-q_int64):     return tag_v_int64;
  case (-q_float32):   return tag_v_float32;
  case (-q_float64):   return tag_v_float64;
  case (-q_char):      return tag_v_char;
  case (-q_month):     return tag_v_month;
  case (-q_date):      return tag_v_date;
  case (-q_datetime):  return tag_v_datetime;
  case (-q_minute):    return tag_v_minute;
  case (-q_second):    return tag_v_second;
  case (-q_time):      return tag_v_time;
  case (-q_timestamp): return tag_v_timestamp;
  case (-q_timespan):  return tag_v_timespan;
  case (-q_guid):      return tag_v_guid;
  default: {
    caml_failwith("tag_for_vector: impossible tag\n");
  }
  }
}


// Convert K->OCaml
static value q_to_ocaml(const K q_val) {
  const H q_type = q_val->t; 
  switch(q_type) {

  // Scalars

  case q_bool: {
    return (mk_caml_value(tag_bool, Val_bool(q_val->g)));
  }
  case q_byte: {
    return (mk_caml_value(tag_byte, Val_int(q_val->g)));
  }
  case q_int16: {
    return (mk_caml_value(tag_int16, Val_int(q_val->h)));
  }
  case q_int32: 
  case q_month: 
  case q_date: 
  case q_minute: 
  case q_second: 
  case q_time: 
  {
    return (mk_caml_value(tag_for_scalar(q_type), caml_copy_int32(q_val->i)));
  }

  case q_int64: 
  case q_timestamp:  
  case q_timespan: 
  {
    return (mk_caml_value(tag_for_scalar64(q_type), caml_copy_int64(q_val->j)));
  }
  case q_float32: {
    return (mk_caml_value(tag_float32, caml_copy_double(q_val->e)));
  }
  case q_float64: {
    return (mk_caml_value(tag_float64, caml_copy_double(q_val->f)));
  }
  case q_char: {
    return(mk_caml_value(tag_char,  Val_int(q_val->g)));
  }
  case q_symbol: {
    return (mk_caml_value(tag_symbol, caml_copy_string(q_val->s)));
  }
  case q_guid: {
    return (mk_caml_value(tag_guid, caml_copy_string((kU(q_val))->g)));
  }
  case q_datetime: {
    return (mk_caml_value(tag_datetime, caml_copy_double(q_val->f)));
  }

  // Vectors of scalars

  case (-q_bool): 
  case (-q_byte): 
  case (-q_char): {
    return (mk_caml_byte_array(tag_for_vector(q_type), q_val));
  }
  case (-q_int16): {
    return mk_caml_scalar_array(tag_for_vector(q_type), CAML_BA_UINT16, kH(q_val), q_val); 
  }
  case (-q_int32):
  case (-q_month): 
  case (-q_date):
  case (-q_minute):
  case (-q_second):
  case (-q_time): {
    return mk_caml_scalar_array(tag_for_vector(q_type), CAML_BA_INT32, kI(q_val), q_val); 
  }
  case (-q_int64):
  case (-q_timestamp):  
  case (-q_timespan): 
  {
    return mk_caml_scalar_array(tag_for_vector(q_type), CAML_BA_INT64, kJ(q_val), q_val); 
  }
  case (-q_float32): {
    return mk_caml_scalar_array(tag_for_vector(q_type), CAML_BA_FLOAT32, kE(q_val), q_val); 
  }
  case (-q_float64): 
  case (-q_datetime): {
    return mk_caml_scalar_array(tag_for_vector(q_type), CAML_BA_FLOAT64, kF(q_val), q_val); 
  }
  case (-q_symbol): {
    return mk_caml_string_array(q_val);
  }

  // Mixed lists

  case q_mixed_list: {
    return (mk_caml_value(tag_mixed_list, mk_caml_array(q_val)));;
  }

  // Tables

  case q_table:{
    return (mk_caml_table(q_val));
  }

  // Dictionaries

  case q_dict: {
    return (mk_caml_dict(q_val));
  }

  case q_unit: {
    return (Val_int(tag_unit));
  }
  case (-q_guid): {
    caml_failwith("Not yet supported: guid vector");
  }
  case q_lambda: {
    caml_failwith("Not supported: lambda (type 100)");
  }
  case q_operator: {
    caml_failwith("Not supported: q operator (type 102)");
  }
  case q_partial_app: {
    caml_failwith("Not supported: partial application (type 104)");
  }
  case q_error: {
      caml_failwith(q_val->s);
  }
  default: {
    fprintf(stderr, "internal error: q_to_ocaml impossible q type %i\n", q_type);
    caml_failwith("internal error: q_to_ocaml impossible q type");
  }
  }
}

///////////////////////////////////////////////////
// Functions to convert OCaml values to kdb+ values
///////////////////////////////////////////////////

// The kdb+ C interface does not provide functions to build values of type
// month, minute, second. The following will do.

static K mk_int_scalar(const int ty, const int i) 
{
  K result = ki(i);
  result->t = ty;
  return result;
}

static int tag_to_type(const int tag) {
  switch(tag){
  case tag_month:     return (q_month);
  case tag_date:      return (q_date);
  case tag_minute:    return (q_minute);
  case tag_second:    return (q_second);
  case tag_timestamp: return (q_timestamp);
  case tag_timespan:  return (q_timespan);
  default: {
    fprintf(stderr, "tag_to_type: impossible tag %i\n", tag);
    caml_failwith("tag_to_type: impossible tag");
  }
  }
}

static int tag_to_v_type(const int tag) {
  switch(tag){
  case tag_v_bool:      return (-q_bool);
  case tag_v_byte:      return (-q_byte);
  case tag_v_int32:     return (-q_int32);
  case tag_v_float32:   return (-q_float32);
  case tag_v_float64:   return (-q_float64);
  case tag_v_char:      return (-q_char);
  case tag_v_month:     return (-q_month);
  case tag_v_date:      return (-q_date);
  case tag_v_datetime:  return (-q_datetime);
  case tag_v_minute:    return (-q_minute);
  case tag_v_second:    return (-q_second);
  case tag_v_time:      return (-q_time);
  case tag_v_timestamp: return (-q_timestamp);
  case tag_v_timespan:  return (-q_timespan);
  case tag_v_guid:      return (-q_guid);
  default: {
    fprintf(stderr, "tag_to_v_type: impossible tag %i\n", tag);
    caml_failwith("tag_to_v_type: impossible tag");
  }
  }
}


static K mk_scalar_vector(const unsigned elem_size, const int ty, const value v) {
  assert (Is_block(v));

  const value arr = Field(v,0);

  assert (Is_block(arr));
  assert (1 == Caml_ba_array_val(arr)->num_dims);

  const long size = Caml_ba_array_val(arr)->dim[0];
  K list = ktn(ty, size);
  list->u = (short)Int_val(Field(v,1)); // Attribute
  memcpy(list->G0, Caml_ba_array_val(arr), size * elem_size);
  return list;
}


static inline K mk_typed_byte_vector(const int ty, const value v) {
  return(mk_scalar_vector(sizeof(char), ty, v));
}

static inline K mk_int16_vector(const value v) {
  return(mk_scalar_vector(sizeof(short), KH, v));
}

static inline K mk_typed_int32_vector(const int ty, const value v) {
  return(mk_scalar_vector(sizeof(int), ty, v));
}

static inline K mk_typed_int64_vector(const int ty, const value v) {
  return(mk_scalar_vector(sizeof(int64_t), ty, v));
}

static inline K mk_float32_vector(const int ty, const value v) {
  return(mk_scalar_vector(sizeof(float), KE, v));
}

static inline K mk_float64_vector(const int ty, const value v) {
  return(mk_scalar_vector(sizeof(double), ty, v));
}

static K mk_char_vector(const int ty, const value v) {
  assert (Is_block(v));

  const value arr = Field(v,0);

  assert (Is_block(arr));
  assert (1 == Caml_ba_array_val(arr)->num_dims);

  K vec = kp((unsigned char *)Caml_ba_data_val(arr));
  vec->u = (short)Int_val(Field(v,1)); // Attribute
  return vec;
}


static K mk_symbol_vector(const value v) {
  assert (Is_block(v));

  const value arr = Field(v,0);
  assert (Is_block(arr));
  const long count = Wosize_val(arr);
  K list = ktn(KS, 0);
  unsigned i;
  for(i=0; i<count; i++) {
    unsigned char* elem = ss((unsigned char *)String_val(Field(arr,i)));
    js(&list, elem);
  }
  // Note: attribute must be set after calling js
  list->u = (short)Int_val(Field(v,1)); // Attribute
  return list;
}


static K mk_mixed_list(const value v) {
  assert (Is_block(v));

  const long count = Wosize_val(v);
  K list = knk(0);
  unsigned i;
  for(i=0; i<count; i++) {
    jk(&list, ocaml_to_q(Field(v, i)));
  }
  return list;
}


// Convert caml value of type q_val to K value
static K ocaml_to_q(const value val)
{
  if(!Is_block(val)) {
    // Q_unit
    assert(tag_unit == Long_val(val));
    // Note: kdb's C interface does not export a function to create units
    K result = kb(0);
    result->t = 101;
    return result;
  } else {
    value v = Field(val, 0);
    const int tag = Tag_val(val);
    switch(tag) {

    // Scalars

    case tag_bool: {
      return kb(Bool_val(v));
    }
    case tag_byte: {
      return kg(Int_val(v));
    }
    case tag_int16: {
      return kh(Int_val(v));
    }
    case tag_int32: {
      return ki(Int32_val(v));
    }
    case tag_int64: {
      return kj(Int64_val(v));
    }
    case tag_float32: {
      return ke(Double_val(v));
    }
    case tag_float64: {
      return kf(Double_val(v));
    }
    case tag_char: {
      return kc(Int_val(v));
    }
    case tag_symbol: {
      // Note: intern the string to create the symbol
      return ks(ss((unsigned char *)String_val(v)));
    }
    case tag_month: 
    case tag_minute:
    case tag_second: {
	    return mk_int_scalar(tag_to_type(tag), Int32_val(v));
    }
    case tag_date: {
      return(kd(Int32_val(v)));
    }
    case tag_timestamp: {
      return(ktj(-KP, Int64_val(v)));
    }
    case tag_timespan: {
	    return(ktj(-KN, Int64_val(v)));
    }
    case tag_datetime: {
      return kz(Double_val(v));
    }
    case tag_time: {
      return kt(Int32_val(v));
    }

    // Vectors of scalars

    case tag_v_bool:
    case tag_v_byte: {
      return mk_typed_byte_vector(tag_to_v_type(tag), val);
    }
    case tag_v_int16: {
      return mk_int16_vector(val);
    }
    case tag_v_int32:  
    case tag_v_month:  
    case tag_v_date:   
    case tag_v_minute: 
    case tag_v_second: 
    case tag_v_time: {
      return mk_typed_int32_vector(tag_to_v_type(tag), val);
    }  
    case tag_v_int64: {
    case tag_v_timestamp: 
    case tag_v_timespan: 
      return mk_typed_int64_vector(tag_to_v_type(tag), val);
    }
    case tag_v_char: {
      return mk_char_vector(tag_to_v_type(tag), val);
      //K arr = kp((unsigned char *)Data_bigarray_val(v));
      //return kp((unsigned char *)Data_bigarray_val(v));
    }
    case tag_v_float32: { 
      return mk_float32_vector(tag_to_v_type(tag), val);
    }
    case tag_v_float64: 
    case tag_v_datetime: {
      return mk_float64_vector(tag_to_v_type(tag), val);
    }
    case tag_v_symbol: {
      return mk_symbol_vector(val);
    }

    // Mixed lists

    case tag_mixed_list: {
      return mk_mixed_list(v);
    }

    // Tables

    case tag_table: {
      K colnames = ocaml_to_q(Field(v, 0));
      K cols     = ocaml_to_q(Field(v, 1));
      K dict = xD(colnames, cols);
      dict->u = (short)Int_val(Field(v, 2)); // Atribute
      return(xT(dict));
    }
    case tag_dict: {
      K keys   = ocaml_to_q(Field(v, 0));
      K values = ocaml_to_q(Field(v, 1));
      K dict = xD(keys, values);
      dict->u = (short)Int_val(Field(v, 2)); // Atribute
      return(dict);
    }

    case tag_guid: 
    case tag_v_guid: {
      fprintf(stderr, "ocaml_to_q: guids not yet supported\n");
      caml_failwith("ocaml_to_q: guids not yet supported");
    }

    default: {
      fprintf(stderr, "ocaml_to_q: invalid tag %i\n",  tag);
      caml_failwith("ocaml_to_q impossible caml tag");
    }
    }
  }
}

///////////////////////////////////////////////////
// Exported Caml functions to talk to kdb processes
///////////////////////////////////////////////////

CAMLprim value q_connect(value host, value port)
{
  CAMLparam2(host, port);
  const int handle = khp(String_val(host), Int_val(port));
  CAMLreturn(caml_copy_int32(handle));
}

CAMLprim value q_eval_async(value handle, value str)
{
  CAMLparam2(handle, str);

  assert(Is_block(str));

  k(-Int32_val(handle), String_val(str), (K)0);
  CAMLreturn(Val_unit);
}

CAMLprim value q_eval(value handle, value str)
{
  CAMLparam2(handle, str);
  CAMLlocal1(result);

  assert(Is_block(str));

  K reply = k(Int32_val(handle), String_val(str), (K)0);
  if(!reply) {
    caml_failwith("Network error");
  }
  result = q_to_ocaml(reply);
  // Free the memory for 'reply'
  r0(reply);
  CAMLreturn(result);
}


CAMLprim value q_rpc_async(value handle, value str, value val)
{
  CAMLparam3(handle, str, val);

  assert(Is_block(str));

  k(-Int32_val(handle), String_val(str), ocaml_to_q(val), (K)0);
  CAMLreturn(Val_unit);
}

CAMLprim value q_rpc(value handle, value str, value val)
{
  CAMLparam3(handle, str, val);
  CAMLlocal1(result);

  assert(Is_block(str));

  K reply = k(Int32_val(handle), String_val(str), ocaml_to_q(val), (K)0);
  result = q_to_ocaml(reply);
  // Free the memory for 'reply'
  r0(reply);
  CAMLreturn(result);
}


