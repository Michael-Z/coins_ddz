#!/bin/bash

cd $(dirname $0)

# db配置
db_host=127.0.0.1
db_port=3306
db_user=root
db_pwd=hEWHQcsTn8h3Hy4c
db_name=lzmjlog

# 建库
/usr/local/mysql/bin/mysql -h$db_host -P$db_port -u$db_user -p$db_pwd -e "create database if not exists $db_name default character set = utf8mb4;"

# 不分表
IN_FILE=lzmjlog.sql
OUT_FILE=/tmp/lzmjlog.sql.out

cat $IN_FILE |sed "s/\#db\#/$db_name/g" > $OUT_FILE
/usr/local/mysql/bin/mysql -h$db_host -P$db_port -u$db_user -p$db_pwd -e "source $OUT_FILE;"

rm $OUT_FILE

# 按YYYYMMDD分表
IN_FILE=lzmjlog_yyyymmdd.sql
OUT_FILE=/tmp/lzmjlog_yyyymmdd.sql.out

count=0
day_end=`date -d "now 30 days" +%Y%m%d`
until [ $table_index == $day_end ]
do
	table_index=`date -d "now $count days" +%Y%m%d`
	cat $IN_FILE |sed "s/\#db\#/$db_name/g" |sed "s/\#tid\#/$table_index/g" > $OUT_FILE
	/usr/local/mysql/bin/mysql -h$db_host -P$db_port -u$db_user -p$db_pwd -e "source $OUT_FILE;"

	if [ $? -eq 0 ]
	then
		echo "$table_index done."
	else
		echo "$table_index failed."
	fi

	count=$[$count+1]
done

rm $OUT_FILE


# 按UID分表
IN_FILE=lzmjlog_uid.sql
OUT_FILE=/tmp/lzmjlog_uid.sql.out

count=0
while [ $count -lt 1 ]
do
	table_index=`printf %03d $count`
	cat $IN_FILE |sed "s/\#db\#/$db_name/g" |sed "s/\#tid\#/$table_index/g" > $OUT_FILE
	/usr/local/mysql/bin/mysql -h$db_host -P$db_port -u$db_user -p$db_pwd -e "source $OUT_FILE;"

	if [ $? -eq 0 ]
	then
		echo "$table_index done."
	else
		echo "$table_index failed."
	fi

	count=$[$count+1]
done

rm $OUT_FILE
