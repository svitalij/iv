// Slot object
// this is holder for lookup Map and determine making this lookup to PIC or not
//
// see also map.h
//
#ifndef IV_LV5_SLOT_H_
#define IV_LV5_SLOT_H_
#include "notfound.h"
#include "lv5/property.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/jsfunction.h"
namespace iv {
namespace lv5 {

class Slot {
 public:
  Slot()
    : cacheable_(false),
      base_(NULL),
      desc_(JSUndefined),
      offset_(core::kNotFound) {
  }

  bool IsCachable() const {
    return cacheable_;
  }

  void set_descriptor(const PropertyDescriptor& desc) {
    cacheable_ = false;
    base_ = NULL;
    desc_ = desc;
    offset_ = core::kNotFound;
  }

  void set_descriptor(const PropertyDescriptor& desc,
                      const JSObject* obj, std::size_t offset) {
    cacheable_ = true;
    base_ = obj;
    desc_ = desc;
    offset_ = offset;
  }

  JSVal Get(Context* ctx, JSVal this_binding, Error* e) const {
    if (desc_.IsDataDescriptor()) {
      return desc_.AsDataDescriptor()->value();
    } else {
      assert(desc_.IsAccessorDescriptor());
      JSObject* const getter = desc_.AsAccessorDescriptor()->get();
      if (getter) {
        ScopedArguments a(ctx, 0, IV_LV5_ERROR(e));
        return getter->AsCallable()->Call(&a, this_binding, e);
      } else {
        return JSUndefined;
      }
    }
  }

  const JSObject* base() const {
    return base_;
  }

  const PropertyDescriptor& desc() const {
    return desc_;
  }

  std::size_t offset() const {
    return offset_;
  }

 private:
  bool cacheable_;
  const JSObject* base_;
  PropertyDescriptor desc_;
  std::size_t offset_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_SLOT_H_
