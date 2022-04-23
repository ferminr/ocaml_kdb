open Bigarray

type char_bigarray =    (char, int8_unsigned_elt, c_layout) Array1.t
type uint8_bigarray =   (int, int8_unsigned_elt, c_layout) Array1.t
type uint16_bigarray =  (int, int16_unsigned_elt, c_layout) Array1.t
type int32_bigarray =   (int32, int32_elt, c_layout) Array1.t
type int64_bigarray =   (int64, int64_elt, c_layout) Array1.t
type float32_bigarray = (float, float32_elt, c_layout) Array1.t
type float64_bigarray = (float, float64_elt, c_layout) Array1.t

(** Q attributes for composite Q values *)

type attrib = 
  | A_none
  | A_s
  | A_u
  | A_p
  | A_g

(** The type of Q values *)
(* Note: there are no q enumerations. In the q-rpc protocol they are symbol vectors *)
(* Note: lambdas, operators, partial applications (types 100, 102 and 104) are not supported in this version *)

type  q_val = 
  (* scalars *)
  | Bool of bool
  | Byte of int
  | Short of int
  | Int32 of int32
  | Int64 of int64
  | Float32 of float
  | Float64 of float
  | Char of char
  | Symbol of string
  | Month of int32
  | Date of int32
  | Datetime of float
  | Minute of int32
  | Second of int32
  | Time of int32
  | Timestamp of int64
  | Timespan of int64
  | Guid of string
  (* vectors of scalars *)
  | V_bool of uint8_bigarray * attrib
  | V_byte of uint8_bigarray * attrib
  | V_short of uint16_bigarray * attrib
  | V_int32 of int32_bigarray * attrib
  | V_int64 of int64_bigarray * attrib
  | V_float32 of float32_bigarray * attrib
  | V_float64 of float64_bigarray * attrib
  | V_char of char_bigarray * attrib
  | V_symbol of string array * attrib
  | V_month of int32_bigarray * attrib
  | V_date of int32_bigarray * attrib
  | V_datetime of float64_bigarray * attrib
  | V_minute of int32_bigarray * attrib
  | V_second of int32_bigarray * attrib
  | V_time of int32_bigarray * attrib
  | V_timestamp of int64_bigarray * attrib
  | V_timespan of  int64_bigarray * attrib
  | V_guid of string array * attrib
  (* mixed lists *)
  | V_mixed of q_val array
  (* tables and dictionaries *)
  | Table of q_table
  | Dict of q_dict
  (* result of Q functions that return void. In q, (::) of type 101 *)
  | Unit


(* Note: types q_dict and q_table could be made abstract, as the q_vals inside them
   cannot be arbitrary and must satisfy invariants *)

and q_dict = { keys: q_val; vals: q_val; attrib_d: attrib }

and q_table = { colnames: q_val; (* Always a Q_v_symbol *)
                cols: q_val;
		attrib_t: attrib }


type q_conn (* abstract *)

(**  an exception to signal connection errors, such as unknown host, connection refused, or connection timeout *)
exception Q_connect of string

val open_connection : string -> int -> q_conn

external eval_async : q_conn -> string -> unit = "q_eval_async"

external eval : q_conn -> string -> q_val = "q_eval"

external rpc_async : q_conn -> string -> q_val -> unit = "q_rpc_async"

external rpc : q_conn -> string -> q_val -> q_val = "q_rpc"



