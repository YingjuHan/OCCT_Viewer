const { defineConfig } = require('@vue/cli-service')
const path = require('path')
const fs = require("fs")

module.exports = defineConfig({
  transpileDependencies: true,
  productionSourceMap: true,
  configureWebpack: {
    devtool: 'source-map'
  },
  chainWebpack: config => {
    config.plugin('copy')
          .tap(args => {
            let path1 = path.resolve(__dirname, 'node_modules/@hyj/SCwebviewer/wasm32');
            if (fs.existsSync(path1)) {
              args[0].patterns.push({
                from: path1,
                to: path.resolve(__dirname, 'dist/others/wasm32'),
                toType: 'dir'
              })
            }
            let path2 = path.resolve(__dirname, 'node_modules/@hyj/SCwebviewer/wasm32-pthread');
            if (fs.existsSync(path2)) {
              args[0].patterns.push({
                from: path2,
                to: path.resolve(__dirname, 'dist/others/wasm32-pthread'),
                toType: 'dir'
              })
            }
            let path3 = path.resolve(__dirname, '../../resources/models');
            if (fs.existsSync(path3)) {
              args[0].patterns.push({
                from: path3,
                to: path.resolve(__dirname, 'dist/models'),
                toType: 'dir'
              })
            }
            let path4 = path.resolve(__dirname, '../../models');
            if (fs.existsSync(path4)) {
              args[0].patterns.push({
                from: path4,
                to: path.resolve(__dirname, 'dist/models'),
                toType: 'dir'
              })
            }
            return args
          })
        }
})
