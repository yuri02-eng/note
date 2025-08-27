const {merge} = require('webpack-merge');
const commonConfig = require('./webpack.config.common');
const devConfig = require('./webpack.config.dev');
const prodConfig = require('./webpack.config.prod');
module.exports = (env) => {
    if (env.development) {
        return merge(commonConfig, devConfig);
    }
    if (!env.development) {
        return merge(commonConfig, prodConfig);
    }
    return new Error('请设置环境变量');
}