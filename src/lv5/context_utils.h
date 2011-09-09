#ifndef IV_LV5_CONTEXT_UTILS_H_
#define IV_LV5_CONTEXT_UTILS_H_
#include "ustring.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "lv5/class.h"
#include "lv5/symbol.h"
#include "lv5/specialized_ast.h"
namespace iv {
namespace lv5 {

class JSVal;
class JSString;
class Context;
class JSRegExpImpl;
class JSFunction;
class GlobalData;
class Map;

namespace context {

const ClassSlot& GetClassSlot(const Context* ctx, Class::JSClassType type);

Symbol Intern(Context* ctx, const core::StringPiece& str);
Symbol Intern(Context* ctx, const core::UStringPiece& str);
Symbol Intern(Context* ctx, const JSString* str);
Symbol Intern(Context* ctx, uint32_t index);
Symbol Intern(Context* ctx, double number);

GlobalData* Global(Context* ctx);

JSString* EmptyString(Context* ctx);
JSString* LookupSingleString(Context* ctx, uint16_t ch);

Map* GetEmptyObjectMap(Context* ctx);
Map* GetFunctionMap(Context* ctx);
Map* GetArrayMap(Context* ctx);
Map* GetStringMap(Context* ctx);
Map* GetBooleanMap(Context* ctx);
Map* GetNumberMap(Context* ctx);
Map* GetDateMap(Context* ctx);
Map* GetRegExpMap(Context* ctx);
Map* GetErrorMap(Context* ctx);

JSFunction* throw_type_error(Context* ctx);

bool IsStrict(const Context* ctx);

void RegisterLiteralRegExp(Context* ctx, JSRegExpImpl* reg);

JSVal* StackGain(Context* ctx, std::size_t size);
void StackRelease(Context* ctx, std::size_t size);

} } }  // namespace iv::lv5::context
#endif  // IV_LV5_CONTEXT_UTILS_H_
