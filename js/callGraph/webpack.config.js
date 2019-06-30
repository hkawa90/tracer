const path = require('path');

// [定数] webpack の出力オプションを指定します
// 'production' か 'development' を指定
const MODE = "development";

// ソースマップの利用有無(productionのときはソースマップを利用しない)
const enabledSourceMap = MODE === "development";

module.exports = {
  target: 'node',
  // モード値を production に設定すると最適化された状態で、
  // development に設定するとソースマップ有効でJSファイルが出力される
  mode: MODE,
  // メインとなるJavaScriptファイル（エントリーポイント）
  entry: './src/index.js',
  // ファイルの出力設定
  output: {
    //  出力ファイルのディレクトリ名
    path: path.resolve(__dirname, 'dist'),
    // 出力ファイル名
    filename: 'main.js'
  },
  resolve: {
    modules: [
      'node_modules',
       path.join(__dirname, 'src'),
    ],
  },
};
