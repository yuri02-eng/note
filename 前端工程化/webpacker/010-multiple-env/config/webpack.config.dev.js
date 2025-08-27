module.exports = {
    output: {
        filename: 'scripts/[name].js',
    },
    mode:  'development',
    devtool: "inline-source-map", // to show the error in the source code
    devServer: {
        static: './dist'
    },
}
