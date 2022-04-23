/*
 * Copyright (c) 2022 Fermin Reig
 *
 * q_interface.h
 */

#ifndef _Q_INTERFACE_H_
#define	_Q_INTERFACE_H_

#define KXVER 3

#include "k.h"
#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>

enum q_type {
  // scalars
  q_bool        = -KB,
  q_byte        = -KG,
  q_int16       = -KH, /* short */
  q_int32       = -KI,
  q_int64       = -KJ,
  q_float32     = -KE, /* real */
  q_float64     = -KF, /* float */
  q_char        = -KC,
  q_symbol      = -KS,
  q_date        = -KD,
  q_month       = -KM,
  q_datetime    = -KZ,
  q_minute      = -KU,
  q_second      = -KV,
  q_time        = -KT,
  q_timestamp   = -KP,
  q_timespan    = -KN,
  q_guid        = -UU, /* 16 bytes */
  // vectors of scalars. (not represented explicitly: use the negated scalar)
  // mixed lists
  q_mixed_list  = 0,
  // tables and dictionaries
  q_table       = XT,
  q_dict        = XD,
  // misc
  q_unit        = 101,
  // lambda, operator, partial app: not yet implemented
  q_lambda      = 100,
  q_operator    = 102,
  q_partial_app = 104,
  q_error       = -128
};

enum caml_tag {
  // scalars 
  tag_bool,
  tag_byte,        
  tag_int16,       
  tag_int32,       
  tag_int64,       
  tag_float32,     
  tag_float64,     
  tag_char,        
  tag_symbol,
  tag_month,       
  tag_date,        
  tag_datetime,    
  tag_minute,      
  tag_second,      
  tag_time,
  tag_timestamp,
  tag_timespan,
  tag_guid,        
  // vectors of scalars
  tag_v_bool,
  tag_v_byte,
  tag_v_int16, 
  tag_v_int32,  
  tag_v_int64,  
  tag_v_float32, 
  tag_v_float64, 
  tag_v_char,
  tag_v_symbol,
  tag_v_month,
  tag_v_date,
  tag_v_datetime, 
  tag_v_minute, 
  tag_v_second,
  tag_v_time,
  tag_v_timestamp,
  tag_v_timespan,
  tag_v_guid,        
  // mixed lists
  tag_mixed_list,  
  // tables and dictionaries
  tag_table,       
  tag_dict,
  // result of Q functions that return void
  // Implementation note: caml constant constructors are numbered separately
  // from non-constant ones
  tag_unit = 0
};


#endif /* _Q_INTERFACE_H_ */
