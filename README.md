# Simple Kv datas
这是使用epoll的一个分支，写的很菜，请大佬勿喷。  
客户端的设置都放在`settings.h`里面了，有需要可以直接修改。  
在Linux arm64上面开发的，不知道在其他架构上会不会有bug。
> 当前分支重构了整个模型

**具体架构：**  
[架构图](P1.png)  

**通信协议**
### Header：
| Field | Length | Type | Meaning |
|-|-|-|-|
| MagicNumber | 4 bytes | UInt32 | 魔数（固定为1234）|
| Size | 4 bytes | UInt32 | 包大小，不包括头部 |
| Type | 4 bytes | UInt32 | 包类型 |
| Padding | 4 bytes | UInt32 | 填充（固定为0）|

### Types:
| Number | Meaning |
|-|-|
| 0 | Put请求 |
| 1 | Delete请求 |
| 2 | Get请求 |
| 3 | Put响应 |
| 4 | Delete响应 |
| 5 | Get响应 |

### Put Request Body:
| Field | Length | Type | Meaning |
|-|-|-|-|
| Key Size | 4 bytes | UInt32 | Key的长度 |
| Key | N bytes | Byte[] | Key |
| Value Size | 4 bytes | UInt32 | Value的长度 |
| Value | N bytes | Byte[] | Value |

### Delete Request Body:
| Field | Length | Type | Meaning |
|-|-|-|-|
| Key Size | 4 bytes | UInt32 | Key的长度 |
| Key | N bytes | Byte[] | Key |

### Get Request Body:
| Field | Length | Type | Meaning |
|-|-|-|-|
| Key Size | 4 bytes | UInt32 | Key的长度 |
| Key | N bytes | Byte[] | Key |

### Put Response Body:
| Field | Length | Type | Meaning |
|-|-|-|-|
| Status | 1 byte | Bool | 是否成功 |

### Delete Response Body:
| Field | Length | Type | Meaning |
|-|-|-|-|
| Status | 1 byte | Bool | 是否成功 |

### Get Response Body:
| Field | Length | Type | Meaning |
|-|-|-|-|
| Value Size | 4 bytes | UInt32 | Value的长度 |
| Value | N bytes | Byte[] | Value |

**默认端口**:1434