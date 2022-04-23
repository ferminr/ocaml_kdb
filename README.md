# OCaml client for kdb+

This library enables an OCaml program to interact with a kdb+ process via IPC. 

It provides functions to evaluate q expressions on a kdb+ process and convert the results to OCaml values.

On the OCaml side, the data representation is efficient, using bigarrays for vectors and tables, and raw integers for q types like dates or times.  

The OCaml client for kdb+ is built on top of the C client library for kdb+.

## How to build the library
---

You first need to download the files k.h and c.o from [kx.com](https://code.kx.com/q/interfaces/c-client-for-q/), as they are external dependencies not included in this repository.

To use with the OCaml interactive interpreter type the below in your terminal:

ocamlc -c q.mli  
ocamlc -c q.ml  
ocamlc -c q_interface.c  
ocamlmklib -o q_ocaml c.o q_interface.o q.ml

To use with the native-code Ocaml compiler, type this instead:

ocamlopt -c q.mli  
ocamlopt -c q.ml  
ocamlopt -c q_interface.c


##  How to use the OCaml kdb+ library
---

1. First start a kdb+ server listening on a port:  
<code>$ rlwrap $QHOME/l64/q -p 5001</code>
2. Then start interactive OCaml interpreter and load the library:  
<code>$ rlwrap ocaml q_ocaml.cma</code>
3. Now you can open a connection to the server and send commands (asynchronous messages) or expressions to be evaluated (synchronous messages):  
<code>
    open Q;;  
    open Bigarray;;

    let server = open_connection "localhost"  5001;;  
    val server : q_conn = <abstr>

    eval_async server "xs: til 10";;  
    \- : unit = ()  

    let vector = eval server "xs";;10";;
    val vector : q_val = V_int64 (<abstr>, A_none)

    (* access the int vector component *)  
    let (V_int64 (ints, _)) = vector;;  
    Warning 8 [partial-match]: this pattern-matching is not exhaustive.  
    ...  
    val ints : int64_bigarray = <abstr>  
    ints.{3};;  
    \- : int64 = 3L
</code>  
Here's an example of a select query returning a table:  
<code>
    eval_async server "t: ([] time: 21:00:00.000 21:00:01.000; price: 1.0 1.2)";;  
    \- : unit = ()  

    let table = eval server "select from t where time > 21:00:00";;  
    val table : q_val =
    Table
    {colnames = V_symbol ([|"time"; "price"|], A_none);
      cols = V_mixed [|V_time (<abstr>, A_none); V_float64 (<abstr>, A_none)|];
      attrib_t = A_none}
</code>


## Supported kdb+ types
---

Scalars (integer, float, date, time, ...), vectors of scalars, mixed lists, dictionaries and tables are all supported.

Support for GUIDs is limited to receiving from the kdb+ server (sending not yet supported). Q lambdas, q operators and q partial applications  (types 100, 102 and 104 in q) are not supported. 

