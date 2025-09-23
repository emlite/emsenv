#include <emscripten/em_js.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t Handle;
typedef Handle (*Callback)(Handle, Handle data);

EM_JS_DEPS(emlite, "$wasmTable,$UTF8ToString,$UTF16ToString,$lengthBytesUTF8,$lengthBytesUTF16,$stringToUTF8,$stringToUTF16");

// clang-format off
EM_JS(void, emlite_init_handle_table_impl, (), {
class HandleTable {
    constructor() {
        this._h2e = new Map();
        this._v2h = new Map();
        this._next = 0;
    }

    _newEntry(value) {
        const h = this._next++;
        this._h2e.set(h, { value, refs: 1 });
        this._v2h.set(value, h);
        return h;
    }

    add(value) {
        if (this._v2h.has(value)) {
            const h = this._v2h.get(value);
            ++this._h2e.get(h).refs;
            return h;
        }
        return this._newEntry(value);
    }

    decRef(h) {
        const e = this._h2e.get(h);
        if (!e) return false;

        if (--e.refs === 0) {
            this._h2e.delete(h);
            this._v2h.delete(e.value);
        }
        return true;
    }

    incRef(h) {
        const e = this._h2e.get(h);
        if (e) ++e.refs;
    }

    get(h) { return this._h2e.get(h)?.value; }
    toHandle(value) { return this.add(value); }
    toValue(h) { return this.get(h); }
    has(value) { return this._v2h.has(value); }
    get size() { return this._h2e.size; }
    [Symbol.iterator]() { return this._h2e.values(); }
}

const HANDLE_MAP = new HandleTable();
HANDLE_MAP.add(null);
HANDLE_MAP.add(undefined);
HANDLE_MAP.add(false);
HANDLE_MAP.add(true);
HANDLE_MAP.add(globalThis);
HANDLE_MAP.add(console);
HANDLE_MAP.add(Symbol("_EMLITE_RESERVED_"));
globalThis.EMLITE_VALMAP = HANDLE_MAP;

function normalizeThrown(e) {
  if (e instanceof Error) return e;
  try {
    const err = new Error(String(e));
    if (e && typeof e === "object") {
      if ("name" in e) err.name = e.name;
      if ("code" in e) err.code = e.code;
    }
    err.cause = e;
    return err;
  } catch {
    return new Error("Unknown JS exception");
  }
}

globalThis.normalizeThrown = normalizeThrown;
});

EM_JS(int, emlite_target_impl, (), {
    return 1041;
});

EM_JS(Handle, emlite_val_new_array_impl, (), {
    return EMLITE_VALMAP.add([]);
});

EM_JS(Handle, emlite_val_new_object_impl, (), {
    return EMLITE_VALMAP.add({});
});

EM_JS(char *, emlite_val_typeof_impl, (Handle n), {
    const str = (typeof EMLITE_VALMAP.get(n)) + "\0";
    const len = lengthBytesUTF8(str);
    const buf = _malloc(len);
    stringToUTF8(str, buf, len);
    return buf;
});

EM_JS(
    Handle,
    emlite_val_construct_new_impl,
    (Handle objRef, Handle argv),
    {
            const target = EMLITE_VALMAP.get(objRef);
        const args   = EMLITE_VALMAP.get(argv).map(
            h => EMLITE_VALMAP.get(h)
        );
        let ret;
        try {
          ret = Reflect.construct(target, args);
        } catch (e) {
          ret = normalizeThrown(e);
        }
        return EMLITE_VALMAP.add(ret);
    }
);

EM_JS(
    Handle,
    emlite_val_func_call_impl,
    (Handle func, Handle argv),
    {
            const target = EMLITE_VALMAP.get(func);
        const args   = EMLITE_VALMAP.get(argv).map(
            h => EMLITE_VALMAP.get(h)
        );
        let ret;
        try {
          ret = Reflect.apply(target, undefined, args);
        } catch (e) {
          ret = normalizeThrown(e);
        }
        return EMLITE_VALMAP.add(ret);
    }
);

EM_JS(void, emlite_val_push_impl, (Handle arr, Handle v), {
    try { EMLITE_VALMAP.get(arr).push(v); } catch {}
});

EM_JS(Handle, emlite_val_make_bool_impl, (bool value), {
    return EMLITE_VALMAP.add(!!value);  // 32-bit signed: -2^31 to 2^31-1
});

EM_JS(Handle, emlite_val_make_int_impl, (int value), {
    return EMLITE_VALMAP.add(value | 0);  // 32-bit signed: -2^31 to 2^31-1
});

EM_JS(Handle, emlite_val_make_uint_impl, (unsigned int value), {
    return EMLITE_VALMAP.add(value >>> 0);  // 32-bit unsigned: 0 to 2^32-1
});

EM_JS(Handle, emlite_val_make_bigint_impl, (long long value), {
    return EMLITE_VALMAP.add(BigInt(value));  // 64-bit signed BigInt
});

EM_JS(Handle, emlite_val_make_biguint_impl, (unsigned long long value), {
    let x = BigInt(value); // may be negative due to signed i64 view
    if (x < 0n) x += 1n << 64n; // normalize to [0, 2^64-1]
    return EMLITE_VALMAP.add(x);  // 64-bit unsigned BigInt
});

EM_JS(Handle, emlite_val_make_double_impl, (double t), {
    return EMLITE_VALMAP.add(t);
});

EM_JS(
    Handle,
    emlite_val_make_str_impl,
    (const char *str, size_t len),
    { return EMLITE_VALMAP.add(UTF8ToString(str, len)); }
);

EM_JS(
    Handle,
    emlite_val_make_str_utf16_impl,
    (const uint16_t *str, size_t len),
    {
        return EMLITE_VALMAP.add(UTF16ToString(str, len));
    }
);

EM_JS(bool, emlite_val_get_value_bool_impl, (Handle n), {
    return (EMLITE_VALMAP.get(n) ? 1 : 0);
});

EM_JS(int, emlite_val_get_value_int_impl, (Handle n), {
    const val = EMLITE_VALMAP.get(n);
    if (typeof val === 'bigint') {
        // Preserve lower 32 bits and signedness without precision loss
        return Number(BigInt.asIntN(32, val));
    }
    return val | 0;  // 32-bit signed conversion
});

EM_JS(unsigned int, emlite_val_get_value_uint_impl, (Handle n), {
    const val = EMLITE_VALMAP.get(n);
    if (typeof val === 'bigint') {
        // Preserve lower 32 bits as unsigned without precision loss
        return Number(BigInt.asUintN(32, val));
    }
    return val >>> 0;  // 32-bit unsigned conversion
});

EM_JS(long long, emlite_val_get_value_bigint_impl, (Handle h), {
    const v = EMLITE_VALMAP.get(h);
    if (typeof v === "bigint") return v; // already BigInt
    return BigInt(Math.trunc(Number(v))); // coerce number → BigInt
});

EM_JS(unsigned long long, emlite_val_get_value_biguint_impl, (Handle h), {
    const v = EMLITE_VALMAP.get(h);
    if (typeof v === "bigint") return v >= 0n ? v : 0n; // clamp negative
    const n = Math.trunc(Number(v));
    return BigInt(n >= 0 ? n : 0); // clamp to unsigned
});

EM_JS(double, emlite_val_get_value_double_impl, (Handle n), {
    return Number(EMLITE_VALMAP.get(n));
});

EM_JS(char*, emlite_val_get_value_string_impl, (Handle n), {
  const val = EMLITE_VALMAP.get(n);
  if (!val || typeof val !== "string") return 0;
  const len = lengthBytesUTF8(val) + 1;
  const buf = _malloc(len);
  stringToUTF8(val, buf, len);
  return buf;
});

EM_JS(unsigned short *, emlite_val_get_value_string_utf16_impl, (Handle n), {
  const val = EMLITE_VALMAP.get(n);
  if (!val || typeof val !== "string") return 0;
  const byteLen = lengthBytesUTF16(val) + 2;
  const buf = _malloc(byteLen);
  stringToUTF16(val, buf, byteLen);
  return buf;
});

EM_JS(Handle, emlite_val_get_impl, (Handle n, Handle idx), {
    return EMLITE_VALMAP.add(EMLITE_VALMAP.get(n)[EMLITE_VALMAP.get(idx)]);
});

EM_JS(void, emlite_val_set_impl, (Handle n, Handle idx, Handle val), {
    EMLITE_VALMAP.get(n)[EMLITE_VALMAP.get(idx)] = EMLITE_VALMAP.get(val);
});

EM_JS(bool, emlite_val_has_impl, (Handle n, Handle idx), {
    try {
        return Reflect.has(EMLITE_VALMAP.get(n), EMLITE_VALMAP.get(idx));
    } catch {
        return false;
    }
});

EM_JS(bool, emlite_val_is_string_impl, (Handle h), {
    const obj            = EMLITE_VALMAP.get(h);
    return typeof obj === "string" || obj instanceof String;
});

EM_JS(bool, emlite_val_is_number_impl, (Handle arg), {
    const obj = EMLITE_VALMAP.get(arg);
    return typeof obj === "number" || obj instanceof Number;
});

EM_JS(bool, emlite_val_is_bool_impl, (Handle h), {
    const v = EMLITE_VALMAP.get(h);
    return ((typeof v === "boolean") || (v instanceof Boolean)) | 0;
});

EM_JS(bool, emlite_val_not_impl, (Handle h), {
    return !EMLITE_VALMAP.get(h);
});

EM_JS(bool, emlite_val_gt_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) > EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_gte_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) >= EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_lt_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) < EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_lte_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) <= EMLITE_VALMAP.get(b);
});

EM_JS(bool, emlite_val_equals_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) == EMLITE_VALMAP.get(b);
});

EM_JS(
    bool,
    emlite_val_strictly_equals_impl,
    (Handle a, Handle b),
    { return EMLITE_VALMAP.get(a) === EMLITE_VALMAP.get(b); }
);

EM_JS(bool, emlite_val_instanceof_impl, (Handle a, Handle b), {
    return EMLITE_VALMAP.get(a) instanceof EMLITE_VALMAP.get(b);
});

EM_JS(void, emlite_val_throw_impl, (Handle arg), { throw arg; });

EM_JS(
    Handle,
    emlite_val_obj_call_impl,
    (Handle obj, const char *name, size_t len, Handle argv),
    {
            const target = EMLITE_VALMAP.get(obj);
        const method = UTF8ToString(name, len);
        const args   = EMLITE_VALMAP.get(argv).map(
            h => EMLITE_VALMAP.get(h)
        );
        let ret;
        try {
          ret = Reflect.apply(target[method], target, args);
        } catch (e) {
          ret = normalizeThrown(e);
        }
        return EMLITE_VALMAP.add(ret);
    }
);

EM_JS(
    bool,
    emlite_val_obj_has_own_prop_impl,
    (Handle obj, const char *prop, size_t len),
    {
            const target = EMLITE_VALMAP.get(obj);
        const p      = UTF8ToString(prop, len);
        return Object.prototype.hasOwnProperty.call(
            target, p
        );
    }
);

EM_JS(Handle, emlite_val_make_callback_impl, (Handle fidx, Handle data), {
    const jsFn = (... args) => {
        const arrHandle =
            EMLITE_VALMAP.add(args.map(v => v));
        let ret;
        try {
            ret = wasmTable.get(fidx)(arrHandle, data);
        } catch (e) {
            ret = normalizeThrown(e);
        }
        return ret;
    };
    return EMLITE_VALMAP.add(jsFn);
});

EM_JS(void, emlite_print_object_map_impl, (), {
    console.log(EMLITE_VALMAP);
});

EM_JS(void, emlite_reset_object_map_impl, (), {
    for (const h of[... EMLITE_VALMAP._h2e.keys()]) {
        if (h > 6) {
            const value = EMLITE_VALMAP._h2e.get(h).value;

            EMLITE_VALMAP._h2e.delete(h);
            EMLITE_VALMAP._v2h.delete(value);
        }
    }
});

EM_JS(void, emlite_val_inc_ref_impl, (Handle h), {
    EMLITE_VALMAP.incRef(h);
});

EM_JS(void, emlite_val_dec_ref_impl, (Handle h), {
    if (h > 6) EMLITE_VALMAP.decRef(h);
});
// clang-format on

EMSCRIPTEN_KEEPALIVE
void emlite_init_handle_table() { emlite_init_handle_table_impl(); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_new_array() { return emlite_val_new_array_impl(); }

EMSCRIPTEN_KEEPALIVE
int emlite_target() { return emlite_target_impl(); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_new_object() { return emlite_val_new_object_impl(); }

EMSCRIPTEN_KEEPALIVE
char *emlite_val_typeof(Handle n) { return emlite_val_typeof_impl(n); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_construct_new(Handle objRef, Handle argv) {
    return emlite_val_construct_new_impl(objRef, argv);
}

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_func_call(Handle func, Handle argv) {
    return emlite_val_func_call_impl(func, argv);
}

EMSCRIPTEN_KEEPALIVE
void emlite_val_push(Handle arr, Handle v) { emlite_val_push_impl(arr, v); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_make_bool(bool value) { return emlite_val_make_bool_impl(value); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_make_int(int value) { return emlite_val_make_int_impl(value); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_make_uint(unsigned int value) { return emlite_val_make_uint_impl(value); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_make_bigint(long long value) { return emlite_val_make_bigint_impl(value); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_make_biguint(unsigned long long value) {
    return emlite_val_make_biguint_impl(value);
}

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_make_double(double t) { return emlite_val_make_double_impl(t); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_make_str(const char *str, size_t len) {
    return emlite_val_make_str_impl(str, len);
}

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_make_str_utf16(const uint16_t *str, size_t len) {
    return emlite_val_make_str_utf16_impl((const unsigned short *)str, len);
}

EMSCRIPTEN_KEEPALIVE
int emlite_val_get_value_int(Handle n) { return emlite_val_get_value_int_impl(n); }

EMSCRIPTEN_KEEPALIVE
unsigned int emlite_val_get_value_uint(Handle n) { return emlite_val_get_value_uint_impl(n); }

EMSCRIPTEN_KEEPALIVE
long long emlite_val_get_value_bigint(Handle n) { return emlite_val_get_value_bigint_impl(n); }

EMSCRIPTEN_KEEPALIVE
unsigned long long emlite_val_get_value_biguint(Handle n) {
    return emlite_val_get_value_biguint_impl(n);
}

EMSCRIPTEN_KEEPALIVE
double emlite_val_get_value_double(Handle n) { return emlite_val_get_value_double_impl(n); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_get_value_bool(Handle n) { return emlite_val_get_value_bool_impl(n); }

EMSCRIPTEN_KEEPALIVE
char *emlite_val_get_value_string(Handle n) { return emlite_val_get_value_string_impl(n); }

EMSCRIPTEN_KEEPALIVE
uint16_t *emlite_val_get_value_string_utf16(Handle n) {
    return emlite_val_get_value_string_utf16_impl(n);
}

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_get(Handle n, Handle idx) { return emlite_val_get_impl(n, idx); }

EMSCRIPTEN_KEEPALIVE
void emlite_val_set(Handle n, Handle idx, Handle val) { return emlite_val_set_impl(n, idx, val); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_has(Handle n, Handle idx) { return emlite_val_has_impl(n, idx); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_is_string(Handle h) { return emlite_val_is_string_impl(h); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_is_bool(Handle h) { return emlite_val_is_bool_impl(h); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_is_number(Handle h) { return emlite_val_is_number_impl(h); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_not(Handle h) { return emlite_val_not_impl(h); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_gt(Handle a, Handle b) { return emlite_val_gt_impl(a, b); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_gte(Handle a, Handle b) { return emlite_val_gte_impl(a, b); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_lt(Handle a, Handle b) { return emlite_val_lt_impl(a, b); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_lte(Handle a, Handle b) { return emlite_val_lte_impl(a, b); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_equals(Handle a, Handle b) { return emlite_val_equals_impl(a, b); }

EMSCRIPTEN_KEEPALIVE
bool emlite_val_strictly_equals(Handle a, Handle b) {
    return emlite_val_strictly_equals_impl(a, b);
}

EMSCRIPTEN_KEEPALIVE
bool emlite_val_instanceof(Handle a, Handle b) { return emlite_val_instanceof_impl(a, b); }

EMSCRIPTEN_KEEPALIVE
void emlite_val_throw(Handle arg) { emlite_val_throw_impl(arg); }

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_obj_call(Handle obj, const char *name, size_t len, Handle argv) {
    return emlite_val_obj_call_impl(obj, name, len, argv);
}

EMSCRIPTEN_KEEPALIVE
bool emlite_val_obj_has_own_prop(Handle obj, const char *prop, size_t len) {
    return emlite_val_obj_has_own_prop_impl(obj, prop, len);
}

EMSCRIPTEN_KEEPALIVE
Handle emlite_val_make_callback(Handle fidx, Handle data) {
    return emlite_val_make_callback_impl(fidx, data);
}

EMSCRIPTEN_KEEPALIVE
void emlite_print_object_map() { emlite_print_object_map_impl(); }

EMSCRIPTEN_KEEPALIVE
void emlite_reset_object_map() { emlite_reset_object_map_impl(); }

EMSCRIPTEN_KEEPALIVE
void emlite_val_inc_ref(Handle h) { emlite_val_inc_ref_impl(h); }

EMSCRIPTEN_KEEPALIVE
void emlite_val_dec_ref(Handle h) { emlite_val_dec_ref_impl(h); }
