# HarmonyOS NAPI C++ Binding

为HarmonyOS的Native API提供的C++封装库。

HarmonyOS SDK Version：5.1.0

## 功能

- 支持所有NAPI数据类型以及他们的操作
- 支持导出C++静态函数为ArkTs函数
- 支持Native侧调用ArkTs函数

## 用例

DevEco Studio新建Native C++和ArkTs混合工程：

```cpp
// entry/src/main/cpp/napi_init.cpp
#include "napi_framework.h"

NAPI_FUNC(CallNative, 2, {
    double sum = cbInfo[0].as<napi::Number>().asDouble() + cbInfo[1].as<napi::Number>().asDouble();
    return napi::Number::Create(env, sum);
})

NAPI_FUNC(NativeCallArkTs, 1, {
    napi::Number value = napi::Number::Create(env, 2);
    return cbInfo[0].as<napi::Function>().call({value});
})

NAPI_FUNC(NAPI_bindFunction, 2, {
    std::string funcName = cbInfo[0].as<napi::String>().asString();
    napi::Function func = cbInfo[1].as<napi::Function>();
    napi::tools::Reflector::Instance().bindFunc(funcName, func);
    return napi::Number::Create(env, static_cast<int>(napi::tools::Reflector::Instance().boundJSFuncs().size()));
})

NAPI_FUNC(NAPI_unbindFunction, 1, {
    std::string funcName = cbInfo[0].as<napi::String>().asString();
    napi::tools::Reflector::Instance().unbindFunc(funcName);
    return napi::Number::Create(env, 0);
})

NAPI_FUNC(NAPI_nativeCallBoundJSFunc, 0, {
    auto a = cbInfo[0].as<napi::BigInt>();
    auto b = cbInfo[1].as<napi::BigInt>();
    for (const auto &[n, f] : napi::tools::Reflector::Instance().boundJSFuncs()) {
        OH_LOG_WARN(LOG_APP, "Found Funcs: %{public}s, type: %{public}d", n.c_str(), f.value().type());
    }
    return napi::tools::Reflector::Instance().callBoundFunc("NAPI.entry.calculator_add", {a, b});
})
    
NAPI_FUNC(array_test, 2, {
    OH_LOG_WARN(LOG_APP, "typeof cbInfo[0]: %{public}d", cbInfo[0].type());
    napi::Array strs = cbInfo[0].as<napi::Array>();
    for (std::uint32_t i = 0; i < strs.length(); ++i) {
//        napi::Value str = strs[i];
        // 建议对于数组对象，使用get/set来读取/写入数组元素，其用的是napi_get_element，是针对数组的。而不是用下标索引，下标索引用的是napi_get_property，是用于Object的属性的。
        napi::Value str = strs.get(i);
        assert(str.isString());
        OH_LOG_WARN(LOG_APP, "strs[%{public}d] = %{public}s", i, str.as<napi::String>().asString().c_str());
    }
    OH_LOG_WARN(LOG_APP, "typeof cbInfo[1]: %{public}d", cbInfo[1].type());
    napi::Array nums = cbInfo[1].as<napi::Array>();
    std::uint32_t i = 0;
    // 注意这里是获取Object的全部属性，会导致Array的length属性也取出来（因为用的是下标运算，走的是napi_get_property。所以建议使用第一种按照索引遍历的方式。
    for (const auto &[key, num] : nums) {
        OH_LOG_WARN(LOG_APP, "<%{public}s, %{public}d>", key.as<napi::String>().asString().c_str(),
                    num.asValue().as<napi::Number>().asInt64());
        assert(num.asValue().isNumber());
        OH_LOG_WARN(LOG_APP, "nums[%{public}d] = %{public}d", i++, num.asValue().as<napi::Number>().asInt32());
    }
    napi::Array ret = napi::Array::Create(env);
    ret.set(0u, napi::Number::Create(env, 2));
    ret.set(1u, strs);
    ret.set(2u, nums);
    return ret;
})
    
// clang-format off
NAPI_REGISTER_FUNCS_MODULE(
    entry, 1, 0, nullptr, 0,
    NAPI_BIND_FUNC(callNative, nullptr, CallNative, nullptr, nullptr, nullptr, napi_default, nullptr),
    NAPI_BIND_FUNC(nativeCallArkTs, nullptr, NativeCallArkTs, nullptr, nullptr, nullptr, napi_default, nullptr),
    NAPI_BIND_FUNC(NAPI_bindFunction, nullptr, NAPI_bindFunction, nullptr, nullptr, nullptr, napi_default, nullptr),
    NAPI_BIND_FUNC(NAPI_unbindFunction, nullptr, NAPI_unbindFunction, nullptr, nullptr, nullptr, napi_default, nullptr),
    NAPI_BIND_FUNC(NAPI_nativeCallBoundJSFunc, nullptr, NAPI_nativeCallBoundJSFunc, nullptr, nullptr, nullptr, napi_default, nullptr),
    NAPI_BIND_FUNC(array_test, nullptr, array_test, nullptr, nullptr, nullptr, napi_default, nullptr),
)
// clang-format on
```

```ts
// entry/src/main/cpp/types/libentry/Index.d.ts
export const callNative: (a: number, b: number) => number;
export const nativeCallArkTs: (cb :(a: number) => number) => number;

export const NAPI_bindFunction: (funcName: string, func: Function) => number;
export const NAPI_unbindFunction: (funcName: string) => number;
export const NAPI_nativeCallBoundJSFunc: (a: bigint, b: bigint) => bigint;

export const array_test: (strs: string[], nums: number[]) => [number, string[], number[]];
```

```ts
// entry/src/main/ets/pages/Index.ets
import { hilog } from '@kit.PerformanceAnalysisKit';
import entry from 'libentry.so'

@Entry
@Component
struct Index {
  private testObject(): entry.Object {
    let myObject: entry.Object = new entry.Object(123, "这是一个对象");
    return myObject;
  }

  build() {
    Column() {
      Row() {
        Column() {
          Button('callNative')
            .fontSize(15)
            .fontWeight(FontWeight.Medium)
            .margin(10)
            .onClick(() => {
              hilog.info(0, 'test', `Result: ${entry.callNative(2, 3)}`);
            })

          Button('nativeCallArkTS')
            .fontSize(15)
            .fontWeight(FontWeight.Medium)
            .margin(10)
            .onClick(() => {
              hilog.info(0, 'test', `Result: ${entry.nativeCallArkTs((a: number) => {
                return a * 2;
              })}`);
            })

          Button('NAPI: nativeCallBoundJSFunc')
            .fontSize(15)
            .fontWeight(FontWeight.Medium)
            .margin(10)
            .onClick(() => {
              entry.NAPI_bindFunction('NAPI.entry.calculator_add', entry.calculator_add); // 先通知Native层，绑定JS函数
              hilog.info(0, 'test', `Trigger C++ Call NAPI_nativeCallBoundJSFunc: ${entry.NAPI_nativeCallBoundJSFunc(5n, 6n)}`);
              entry.NAPI_unbindFunction('NAPI.calculator_add');
              // hilog.info(0, 'test', `Trigger C++ Call NAPI_nativeCallBoundJSFunc: ${entry.NAPI_nativeCallBoundJSFunc(6, 6)}`); // 这里会挂掉
            }
          })

          Button('array_test')
            .fontSize(15)
            .fontWeight(FontWeight.Medium)
            .margin(10)
            .onClick(() => {
              const result = entry.array_test(["nice", "to", "meet", "you", '!'], [1, 2, 3, 4, 5]);
              hilog.info(0, 'test', JSON.stringify(result, null, 2));
            })
        }.width('100%')
        .height('100%')
      }.width('100%')
      .height('10%')
    }.width('100%').height('100%')
  }
}
```

## FAQ

Q：为什么要有这个库？不是已经有 [nodejs/node-addon-api](https://github.com/nodejs/node-addon-api) 了吗？

A：因为HarmonyOS的NAPI并不与node.js完全一致，体现在API接口、行为上。所以需要单独提供一个。

## TODO

- [ ] 支持C++类导出为JS类
- [ ] 支持更简单的MODULE声明
- [ ] 支持线程安全函数