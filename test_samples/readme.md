# 测试样例

## [文字转二维码](text2QRCode)

包含文字到各类型二维码的合法与不合法测试样例

所有测试均在未启用Base64选项下完成，**除了** [QRCode_invalid.txt](text2QRCode/QRCode_invalid.txt) 该测试启用Base64可通过

* 报错*Unsupported Format*的样例暂时**未**测试是否正确


## [二维码转文字](QRCode2text)

考虑到多数图案本身是正确的，只保留个别大尺度测试用例
* 对于所有QRCode都启用了Base64
* 对于所有Aztex都启用了Base64
