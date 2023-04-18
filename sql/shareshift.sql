#数据库表
#创建数据库
DROP DATABASE IF EXISTS `shareshift.index`;
CREATE DATABASE `shareshift.index`;

#使用数据库
use `shareshift.index`;


DROP TABLE IF EXISTS `file_info`;
CREATE TABLE `file_info` (
                             `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '文件序号，自动递增，主键',
                             `md5` varchar(256) NOT NULL COMMENT '文件md5',
                             `file_id` varchar(256) NOT NULL COMMENT '文件id:/group1/M00/00/00/xxx.png',
                             `url` varchar(512) NOT NULL COMMENT '文件url 192.168.52.139:80/group1/M00/00/00/xxx.png',
                             `size` bigint(20) DEFAULT '0' COMMENT '文件大小, 以字节为单位',
                             `type` varchar(32) DEFAULT '' COMMENT '文件类型： png, zip, mp4……',
                             `count` int(11) DEFAULT '0' COMMENT '文件引用计数,默认为1。每增加一个用户拥有此文件，此计数器+1',
                             PRIMARY KEY (`id`),
    -- UNIQUE KEY `uq_md5` (`md5)
                             KEY `uq_md5` (`md5`(8))  -- 前缀索引
) ENGINE=InnoDB AUTO_INCREMENT=70 DEFAULT CHARSET=utf8 COMMENT='文件信息表';


DROP TABLE IF EXISTS `share_file_list`;
CREATE TABLE `share_file_list` (
                                   `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '编号',
                                   `user` varchar(32) NOT NULL COMMENT '文件所属用户',
                                   filemd5 varchar(256) NOT NULL COMMENT '文件md5',
                                   `urlmd5` varchar(256) NOT NULL COMMENT '文件urlmd5',
                                   `file_name` varchar(128) DEFAULT NULL COMMENT '文件名字',
                                   `key` varchar(8) NOT NULL COMMENT '提取码',
                                   `pv` int(11) DEFAULT '1' COMMENT '文件下载量，默认值为1，下载一次加1',
                                   `create_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP COMMENT '文件共享时间',
                                   PRIMARY KEY (`id`),
                                   #TODO: 这里index怎么设计，值得思考
                                   key `idx_filename_filemd5_user` (`filemd5`, `user`),
                                   key `idx_filemd5_user` (urlmd5, `user`)
) ENGINE=InnoDB AUTO_INCREMENT=16 DEFAULT CHARSET=utf8 COMMENT='共享文件列表';

DROP TABLE IF EXISTS `user_file_list`;
CREATE TABLE `user_file_list` (
                                  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '编号',
                                  `user` varchar(32) NOT NULL COMMENT '文件所属用户',
                                  `md5` varchar(256) NOT NULL COMMENT '文件md5',
                                  `create_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP COMMENT '文件创建时间',
                                  `file_name` varchar(128) DEFAULT NULL COMMENT '文件名字',
                                  `shared_status` int(11) DEFAULT NULL COMMENT '共享状态, 0为没有共享， 1为共享',
                                  `pv` int(11) DEFAULT NULL COMMENT '文件下载量，默认值为0，下载一次加1',
                                  PRIMARY KEY (`id`),
                                  KEY   `idx_user_md5_file_name` (`user`,`md5`, `file_name`)
) ENGINE=InnoDB AUTO_INCREMENT=30 DEFAULT CHARSET=utf8 COMMENT='用户文件列表';


DROP TABLE IF EXISTS `user_info`;
CREATE TABLE `user_info` (
                             `id` bigint(20) NOT NULL AUTO_INCREMENT COMMENT '用户序号，自动递增，主键',
                             `user_name` varchar(32) NOT NULL DEFAULT '' COMMENT '用户名称',
                             `password` varchar(32) NOT NULL DEFAULT '' COMMENT '密码',
                             `create_time` timestamp NULL DEFAULT CURRENT_TIMESTAMP COMMENT '时间',
                             PRIMARY KEY (`id`),
                             UNIQUE KEY `uq_user_name` (`user_name`)
) ENGINE=InnoDB AUTO_INCREMENT=14 DEFAULT CHARSET=utf8 COMMENT='用户信息表';
