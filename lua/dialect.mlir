Dialect @lua {
  //--------------------------------------------------------------------------//
  // Types
  //--------------------------------------------------------------------------//
  Type @value
  Alias @Value -> !dmc.Isa<@lua::@value> // TODO implicitly buildable
    { builder = "lua.value()" }

  /// Concrete built-in types.
  Alias @bool -> i1
    { builder = "IntegerType(1)" }

  // Lua uses 64-bit integers and floats
  Alias @real -> f64 { builder = "F64Type()" }
  Alias @integer -> i64 { builder = "IntegerType(64)" }
  Alias @number -> !dmc.AnyOf<!lua.real, !lua.integer>

  Alias @concrete -> !dmc.AnyOf<!lua.bool, !lua.number>

  //--------------------------------------------------------------------------//
  // High-Level Ops
  //--------------------------------------------------------------------------//
  Op @add(lhs: !lua.Value, rhs: !lua.Value) -> (res: !lua.Value)
    config { fmt = "`(` operands `)` attr-dict" }
  Op @sub(lhs: !lua.Value, rhs: !lua.Value) -> (res: !lua.Value)
    config { fmt = "`(` operands `)` attr-dict" }

  Op @eq(lhs: !lua.Value, rhs: !lua.Value) -> (res: !lua.Value)
    config { fmt = "`(` operands `)` attr-dict" }
  Op @neq(lhs: !lua.Value, rhs: !lua.Value) -> (res: !lua.Value)
    config { fmt = "`(` operands `)` attr-dict" }

  //--------------------------------------------------------------------------//
  // Concrete Type Ops
  //--------------------------------------------------------------------------//
  Op @get_nil() -> (res: !lua.Value)
    config { fmt = "attr-dict" }
  Op @new_table() -> (res: !lua.Value)
    config { fmt = "attr-dict" }
  Op @get_string() -> (res: !lua.Value) { value = #dmc.String }
    config { fmt = "$value attr-dict" }

  Op @wrap(val: !lua.concrete) -> (res: !lua.Value)
    config { fmt = "$val `:` type($val) attr-dict" }
  Op @unwrap(val: !lua.Value) -> (res: !lua.concrete)
    config { fmt = "$val `->` type($res) attr-dict" }

  //--------------------------------------------------------------------------//
  // Type querying
  //--------------------------------------------------------------------------//
  Op @typeof(val: !lua.Value) -> (res: !lua.Value)
    config { fmt = "$val attr-dict" }

  //--------------------------------------------------------------------------//
  // Table Ops
  //--------------------------------------------------------------------------//
  Op @table_get(tbl: !lua.Value, key: !lua.Value) -> (res: !lua.Value)
    config { fmt = "$tbl `[` $key `]` attr-dict" }
  Op @table_set(tbl: !lua.Value, key: !lua.Value, value: !lua.Value) -> ()
    config { fmt = "$tbl `[` $key `]` `=` $value attr-dict" }
  Op @table_size(tbl: !lua.Value) -> (res: !lua.Value)
    config { fmt = "$tbl attr-dict" }

  //--------------------------------------------------------------------------//
  // Functions
  //--------------------------------------------------------------------------//
  Op @call(arg: !dmc.Variadic<!lua.Value>) -> (res: !dmc.Variadic<!lua.Value>)
    { callee = #dmc.String }
    traits [@SameVariadicOperandSizes, @SameVariadicResultSizes]
    config { fmt = "symbol($callee) `(` operands `)` `:` functional-type(operands, results) attr-dict" }
  Op @ret(arg: !dmc.Variadic<!lua.Value>) -> ()
    traits [@SameVariadicOperandSizes, @IsTerminator]
    config { fmt = "`(` operands `)` `:` type(operands) attr-dict" }

  Op @func() -> () { name = #dmc.String, funcTy = #dmc.Type }
    (body: Sized<1>)
    config { fmt = "symbol($name) $funcTy attr-dict $body" }
}

Dialect @luac {
  Op @wrap_int(val: !lua.integer) -> (res: !lua.Value) config { fmt = "$val attr-dict" }
  Op @wrap_real(val: !lua.real) -> (res: !lua.Value) config { fmt = "$val attr-dict" }
  Op @wrap_bool(val: !lua.bool) -> (res: !lua.Value) config { fmt = "$val attr-dict" }
  Op @unwrap_int(val: !lua.Value) -> (res: !lua.integer) config { fmt = "$val attr-dict" }
  Op @unwrap_real(val: !lua.Value) -> (res: !lua.real) config { fmt = "$val attr-dict" }
  Op @unwrap_bool(val: !lua.Value) -> (res: !lua.bool) config { fmt = "$val attr-dict" }

  Type @string
  Op @load_string() -> (str: !luac.string, len: i32) { value = #dmc.String }
    config { fmt = "$value `->` `(` type(results) `)` attr-dict" }
  Op @get_string(str: !luac.string, len: i32) -> (res: !lua.Value)
    config { fmt = "`(` operands `)` `:` functional-type(operands, results) attr-dict" }
}

Dialect @lualib {
  Op @alloc() -> (res: !lua.Value)
    config { fmt = "attr-dict" }
}

Dialect @luallvm {
  Alias @value -> !llvm<"{ i32, { { { i64 }, i32 } } }*">
  Alias @string -> !llvm<"i8*">
}
