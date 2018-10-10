use #db#;

#用户信息
create table if not exists lzmj_user_info_#tid# (
	uid bigint not null,
	chip bigint default 0,
	exp int default 0,
	name varchar(64) default '',
	gender tinyint default 2,
	pic varchar(256) default '',
	is_online bool default false,
	acc_type int default 0,
    channel int default 0,
	device_name varchar(20),
	band varchar(100),
	time_stamp int,
	regist_stamp int not null,
	diamond int not null,
	bonus float not null comment '现在奖金',
	total_bonus float not null comment '历史总奖金',
	total_win_num int not null,
	primary key(uid),
	key acc_type (acc_type),
	key channel (channel),
	key time_stamp (time_stamp),
	key regist_stamp (regist_stamp),
	key total_bonus(total_bonus),
	key total_win_num(total_win_num)
)engine = InnoDB, charset = utf8mb4;

create table if not exists lzmj_diamond_flow_#tid#(
	id bigint unsigned AUTO_INCREMENT,
	uid bigint not null comment '用户uid',
	num bigint not null comment '操作数',
	total bigint not null comment '操作结果',
	reason int not null comment '操作原因',
	time_stamp int not null,
	acc_type int not null comment '账号类型，游客、机器人、三方...',
	channel int not null comment '渠道号',
	primary key(id),
	key total(total),
	key num(num),
	key uid(uid),
	key channel(channel)
	
)engine = InnoDB, charset = utf8mb4;

create table if not exists lzmj_bonus_flow_#tid#(
	id bigint unsigned AUTO_INCREMENT,
	uid bigint not null comment '用户uid',
	num float not null comment '操作数',
	total float not null comment '操作结果',
	reason int not null comment '操作原因',
	time_stamp int not null,
	acc_type int not null comment '账号类型，游客、机器人、三方...',
	channel int not null comment '渠道号',
	primary key(id),
	key total(total),
	key num(num),
	key uid(uid),
	key channel(channel)
)engine = InnoDB, charset = utf8mb4;

