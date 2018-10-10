use #db#;

#在线表
create table if not exists lzmj_online (
	id int unsigned AUTO_INCREMENT,
	online int not null comment '在线人数',
	play int not null comment '在玩人数',
	time_stamp int not null,
	primary key(id),
	key time_stamp (time_stamp)
)engine = InnoDB, charset = utf8mb4;

#在玩表
create table if not exists lzmj_atplay (
	id int unsigned AUTO_INCREMENT,
	play int not null comment '在玩人数',
	game_type int not null comment '玩法',
	time_stamp int not null,
	primary key(id),
	key time_stamp (time_stamp)
)engine = InnoDB, charset = utf8mb4;