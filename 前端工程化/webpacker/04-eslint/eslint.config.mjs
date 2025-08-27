// 安装依赖：npm i -D eslint @eslint/js typescript-eslint eslint-plugin-vue eslint-plugin-react
import globals from "globals";
import js from "@eslint/js";

export default [
  // 基础规则
  js.configs.recommended,
  {
    languageOptions: {
      parserOptions: {
        ecmaFeatures: { jsx: true },
      },
      globals: { ...globals.browser, ...globals.node },
    },
  },
  // 自定义规则
  {
    rules: {
      "no-console": process.env.NODE_ENV === "production" ? "error" : "warn",
      "quotes": ["error", "single"],
      "semi": ["error", "never"],
      "indent": ["error", 2],
      "vue/multi-word-component-names": "off", // 关闭 Vue 组件名必须多单词
    },
  },
  // 忽略文件
  { ignores: ["dist/**", "node_modules/**"] },
];