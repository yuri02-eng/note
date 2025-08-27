const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const miniCssExtractPlugin = require('mini-css-extract-plugin');
const MiniCssExtractPlugin = require("mini-css-extract-plugin");
const CssMinimizerPlugin = require("css-minimizer-webpack-plugin");
const toml = require('toml')
const yaml = require('yaml')
const json5 = require('json5')
module.exports = {
    entry: {
        // index: {
        //     import: './src/index.js',
        //     dependOn: 'shared'
        // },
        // another:
        //     {
        //     import: './src/another-module.js',
        //     dependOn: 'shared'
        // },
        // shared:'lodash'
        index: './src/index.js',
        another: './src/another-module.js'
        //插件方法抽离共同代码
    },
    output: {
        filename: '[name].bundle.js',
        path: path.resolve(__dirname, 'dist'),
        clean: true,
        assetModuleFilename: 'images/[contenthash][ext]'

    },
    mode: "development",
    devtool: "inline-source-map", // to show the error in the source code
    plugins: [
        new HtmlWebpackPlugin({
            template: "./index.html",
            filename: "app.html",
            inject: "body"
        }),
        new miniCssExtractPlugin({
            filename: "styles/[contenthash].css"
        })
    ],
    devServer: {
        static: './dist'
    },
    module: {
        rules: [
            {
                test: /\.png$/,
                type: "asset/resource",
                generator: {
                    filename: 'images/[contenthash][ext]'
                }
            },
            {
                test: /\.svg$/,
                type: "asset/inline"
            },
            {
                test: /\.txt$/,
                type: "asset/source"
            },
            {
                test: /\.jpg$/,
                type: "asset",
                parser: {
                    dataUrlCondition: {
                        maxSize: 4 * 1024 * 1024 // 4Mb
                    }
                }
            },
            {
                test: /\.(css|less)$/,
                use: [MiniCssExtractPlugin.loader, "css-loader", "less-loader"]// css-loader is used to parse the css file and style-loader is used to inject the css file into the html file
                //顺序不能颠倒，因为是链式调用的，从后往前
            },
            {
                test: /\.(woff|woff2|eot|ttf|otf)$/,
                type: "asset/resource",
            },
            {
                test: /\.(csv|tsv)/,
                use: "csv-loader",
            },
            {
                test: /\.xml/,
                use: "xml-loader",
            },
            {
                test: /.toml$/,
                type: "json",
                parser: {
                    parse: toml.parse
                }
            },
            {
                test: /.yaml$/,
                type: "json",
                parser: {
                    parse: yaml.parse
                }
            },
            {
                test: /.json5$/,
                type: "json",
                parser: {
                    parse: json5.parse
                }
            },
            {
                test: /\.js$/,
                exclude: /node_modules/,
                use: {
                    loader: "babel-loader",
                    options: {
                        presets: ['@babel/preset-env'],
                        plugins: ["@babel/plugin-transform-runtime"]
                    }
                }
            },
        ]
    },
    optimization: {
        minimize: true,
        minimizer: [
            new CssMinimizerPlugin()
        ],
        splitChunks: {
            chunks: "all",
        }
    }
};