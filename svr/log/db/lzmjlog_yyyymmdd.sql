use #db#;

#筹码流水
create table if not exists lzmj_chip_#tid# (
	id bigint unsigned AUTO_INCREMENT,
	uid bigint not null comment '用户uid',
	num bigint not null comment '操作数',
	total bigint not null comment '操作结果',
	reason int not null comment '操作原因',
	time_stamp int not null,
	sign tinyint not null comment '1发放/0回收',
	acc_type int not null comment '账号类型，游客、机器人、三方...',
    channel int default 0,
	primary key(id),
	key reason (reason),
	key sign (sign),
	key uid (uid),
	key reason_acc_type_channel_uid(reason,acc_type,channel,uid),
	key acc_type(acc_type)
)engine = InnoDB, charset = utf8mb4;

#注册表
create table if not exists lzmj_regist_#tid# (
	uid bigint not null,
	time_stamp int not null,
	acc_type int not null,
    channel int default 0,
	device_name varchar(20),
	band varchar(100),
	primary key(uid),
	key acc_type_device_name_channel(acc_type,device_name,channel),
	key acc_type (acc_type),
	key time_stamp(time_stamp)
)engine = InnoDB, charset = utf8mb4;

#登录表
create table if not exists lzmj_login_#tid# (
	uid bigint not null,
	time_stamp int not null,
	acc_type int not null,
    channel int default 0,
	device_name varchar(20),
	band varchar(100),
	login_times int default 1,
	primary key(uid),
	key acc_type (acc_type),
	key acc_type_device_name_channel(acc_type,device_name,channel),
	key channel(channel)
)engine = InnoDB, charset = utf8;

create table if not exists lzmj_game_table_#tid# (
	id int unsigned AUTO_INCREMENT,
	game_id bigint not null,
	table_id int not null,
	table_log TEXT(409600) not null,
	begin_time int not null,
	end_time int not null,
	seat_num int not null,
	primary key(id),
	key game_id (game_id),
	key seat_num (seat_num)
)engine = InnoDB, charset = utf8;

#牌局玩家日志
create table if not exists lzmj_game_player_#tid# (
	id int unsigned AUTO_INCREMENT,
	uid bigint not null,
	game_id bigint not null,
	table_id int not null,
	time_stamp int not null,
	acc_type int not null,
	game_type int not null,
	seat_num int not null,
	is_free_game int not null,
	is_finished_game int not null,
	channel int not null comment '渠道号',
	primary key(id),
	key uid (uid),
	key game_type (game_type),
	key seat_num (seat_num),
	key game_id (game_id),
	key is_free_game(is_free_game),
	key is_finished_game(is_finished_game),
	key channel(channel)
)engine = InnoDB, charset = utf8;

#玩牌统计
create table if not exists lzmj_game_info_#tid# (
	id int unsigned AUTO_INCREMENT,
	game_id bigint not null,
	game_type int not null,
	conf_round int not null,
	real_round int not null,
	seat_num int not null,
	create_uid int not null,
	master_uid int not null,
	primary key(id),
	key game_id (game_id),
	key create_uid (create_uid),
	key master_uid (master_uid),
	key game_type (game_type)
)engine = InnoDB, charset = utf8;

#茶馆房卡流水
create table if not exists lzmj_tea_bar_chips_#tid# (
	id bigint unsigned AUTO_INCREMENT,
	tbid bigint not null comment '茶馆id',
	master_uid bigint not null comment '群主id',
	num bigint not null comment '操作数',
	total bigint not null comment '操作结果',
	reason int not null comment '操作原因',
	time_stamp int not null,
	primary key(id),
	key reason (reason),
	key tbid (tbid),
	key master_uid (master_uid)
)engine = InnoDB, charset = utf8mb4;

# 牌桌统计
create table if not exists lzmj_table_info_#tid# (
	id int unsigned AUTO_INCREMENT,
	game_id bigint not null,
	table_id int not null,
	seat_num int not null,
	conf_round int not null,
	real_round int not null,
	game_type int not null,
	detail varchar(100) not null comment '战绩（大结算）',
	creator_uid bigint default 0 comment '群主id',
	tbid bigint default 0 comment '茶馆id',
	master_uid bigint default 0 comment '群主id',
	time_used int not null comment '耗时',
	time_stamp int not null,

	primary key(id),
	key game_id (game_id),
	key seat_num (seat_num),
	key conf_round (conf_round),
	key game_type (game_type),
	key tbid (tbid),
	key master_uid (master_uid),
	key creator_uid (creator_uid)
)engine = InnoDB, charset = utf8;

# 牌局玩家日志
create table if not exists lzmj_table_player_#tid# (
	id int unsigned AUTO_INCREMENT,
	uid bigint not null,
	game_id bigint not null,
	win_round int not null comment '赢了局数',
	score int not null comment '最终得分',
	cost int not null comment '房卡消耗',
	is_winner tinyint not null comment '是否为大赢家',
	time_stamp int not null,
	
	primary key(id),
	key uid (uid),
	key game_id (game_id),
	key is_winner (is_winner)
)engine = InnoDB, charset = utf8;

# 创建房间统计
create table if not exists lzmj_create_table_#tid# (
	id int unsigned AUTO_INCREMENT,
	creator_uid bigint not null,
	master_uid bigint not null,
	game_type int not null,
	table_id int not null,
	primary key(id),
	key creator_uid (creator_uid),
	key master_uid (master_uid),
	key game_type (game_type)
)engine = InnoDB, charset = utf8;

#闯关等级状态表
create table if not exists lzmj_skip_match_info_#tid#(
	id bigint unsigned AUTO_INCREMENT,
	uid bigint not null comment '用户uid',
	level_change_num int not null comment '等级改变',
	level_total int not null comment '等级改变后结果',
	is_finished_skip int not null comment '是否是通关成功',
	state int not null comment '状态变更后',
	game_type int not null comment '游戏类型',
	reason int not null comment '操作原因',
	channel int not null comment '渠道号',
	time_stamp int not null,
	primary key(id),
	key reason(reason),
	key uid(uid),
	key game_type(game_type),
	key channel(channel)
)engine = InnoDB, charset = utf8mb4;

#金币流水
create table if not exists lzmj_coin_#tid# (
	id bigint unsigned AUTO_INCREMENT,
	uid bigint not null comment '用户uid',
	num bigint not null comment '操作数',
	total bigint not null comment '操作结果',
	reason int not null comment '操作原因',
	time_stamp int not null,
	sign tinyint not null comment '1发放/0回收',
	acc_type int not null comment '账号类型，游客、机器人、三方...',
    channel int default 0,
	primary key(id),
	key reason (reason),
	key sign (sign),
	key uid (uid),
	key reason_acc_type_channel_uid(reason,acc_type,channel,uid),
	key acc_type(acc_type)
)engine = InnoDB, charset = utf8mb4;