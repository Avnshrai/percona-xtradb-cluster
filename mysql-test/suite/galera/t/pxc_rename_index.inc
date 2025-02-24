#
# This test has been taken from innodb/t/innodb_rename_index.test and modified
# as per PXC.
#

#
# Test "ALTER TABLE ... RENAME INDEX" in InnoDB
#

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
let create =
CREATE TABLE t (
	a INT,
	b INT,
	c INT,
	d INT,
	e INT,
	f INT,
	PRIMARY KEY (a),
	INDEX i1 (b),
	INDEX i2 (c),
	INDEX i3 (d),
	INDEX i4 (e)
) ENGINE=INNODB;

let insert = INSERT INTO t SET a = 1;

let show_table =
SHOW CREATE TABLE t;

let show_sys =
SELECT
t.name AS table_name,
i.name AS index_name,
f.name AS column_name
FROM
information_schema.innodb_tables t,
information_schema.innodb_indexes i,
information_schema.innodb_fields f
WHERE
t.name LIKE '%/t' AND
t.table_id = i.table_id AND
i.index_id = f.index_id
ORDER BY 1, 2, 3;

let show_stats =
SELECT index_name, stat_description
FROM mysql.innodb_index_stats
WHERE table_name = 't' AND stat_name LIKE 'n_diff%'
ORDER BY 1, 2;

-- eval $create

# Add a row, so that affected rows would be nonzero for ALGORITHM=COPY.
# ALGORITHM=INPLACE will report 0 affected row in the result file.
# We will have enable_info/disable_info around every successful ALTER
# to enable the affected rows: output in the result file.
-- eval $insert

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- error ER_WRONG_NAME_FOR_INDEX
ALTER TABLE t RENAME INDEX i1 TO GEN_CLUST_INDEX;

# Test all combinations of ADD w, DROP x, RENAME y TO z.
#
# Use the following names for wxyz (with 1 to 4 of wxyz being the same):
# aaaa abcd aabb abab abba abcc acbc accb cacb cabc ccab aaab aaba abaa baaa
#
# Some cases should trivially succeed or fail. Test them in isolation:
# no-op: y=z (RENAME y TO y)
# rules out the combinations ..\(.\)\1
# a.k.a. aaaa aabb abcc abaa baaa

# We use the index names i1 to i4 for existing indexes abcd.
# Non-existing index names will be aa,bb,cc,dd.
# Index creation on non-existing columns will not be tested.

ALTER TABLE t RENAME INDEX i1 TO i1;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t RENAME INDEX aa TO aa;

-- echo # combination: aaaa
# drop/add existing, null rename and drop the same
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i4(f), DROP INDEX i4, RENAME INDEX i4 TO i4;

-- echo # combination: aabb
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX aa, RENAME INDEX i2 TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX aa, RENAME INDEX bb TO bb;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i1, RENAME INDEX bb TO bb;

-- enable_info
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i1, RENAME INDEX i2 TO i2;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

-- echo # combination: abcc

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX bb, RENAME INDEX cc TO cc;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX bb, RENAME INDEX i3 TO i3;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i2, RENAME INDEX cc TO cc;

# rename existing (succeeds)
-- enable_info
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i2, RENAME INDEX i3 TO i3;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

-- echo # combination: abaa

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i1, RENAME INDEX aa TO aa;
-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i2, RENAME INDEX i1 TO i1;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i1(f), DROP INDEX bb, RENAME INDEX i1 TO i1;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX bb, RENAME INDEX aa TO aa;

-- echo # combination: baaa

-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i2(f), DROP INDEX i1, RENAME INDEX i1 TO i1;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX bb(f), DROP INDEX i1, RENAME INDEX i1 TO i1;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i2(f), DROP INDEX aa, RENAME INDEX aa TO aa;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX bb(f), DROP INDEX aa, RENAME INDEX aa TO aa;

# refuse: w=z (ADD w, RENAME y TO w)
# rules out the combinations \(.\)..\1
# a.k.a. aaaa abba cabc aaba abaa
# the case w=y (ADD w, RENAME w to z) may succeed, as seen below

-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX aa(f), RENAME INDEX aa TO bb;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX aa(f), RENAME INDEX bb TO aa;
-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX aa(f), RENAME INDEX i2 TO aa;

# rename existing, add one with the same name
-- enable_info
ALTER TABLE t ADD INDEX i1(f), RENAME INDEX i1 TO bb;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

-- echo # combination: abba

-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i2, RENAME INDEX i2 TO i1;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i2, RENAME INDEX i2 TO aa;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i1(f), DROP INDEX bb, RENAME INDEX bb TO i1;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX bb, RENAME INDEX bb TO aa;

-- echo # combination: cabc

-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX i3(f), DROP INDEX i1, RENAME INDEX i2 TO i3;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i3(f), DROP INDEX aa, RENAME INDEX i2 TO i3;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i3(f), DROP INDEX i1, RENAME INDEX bb TO i3;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i3(f), DROP INDEX aa, RENAME INDEX bb TO i3;

-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX cc(f), DROP INDEX i1, RENAME INDEX i2 TO cc;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX cc(f), DROP INDEX aa, RENAME INDEX i2 TO cc;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX cc(f), DROP INDEX i1, RENAME INDEX bb TO cc;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX cc(f), DROP INDEX aa, RENAME INDEX bb TO cc;

# refuse: x=y (DROP x, RENAME x TO z)
# rules out the combinations .\(.\)\1.
# a.k.a. aaaa abba accb aaab baaa

# rename and drop the same
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t DROP INDEX i1, RENAME INDEX i1 TO bb;
# drop non-existing
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t DROP INDEX aa, RENAME INDEX i2 TO aa;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t DROP INDEX aa, RENAME INDEX aa TO i2;

# this one will succeed (drop, replace with an existing one)
-- enable_info
ALTER TABLE t DROP INDEX i1, RENAME INDEX i4 TO i1;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

-- echo # combination: accb

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i3, RENAME INDEX i3 TO i2;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i3, RENAME INDEX i3 TO bb;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i1(f), DROP INDEX cc, RENAME INDEX cc TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i1(f), DROP INDEX cc, RENAME INDEX cc TO bb;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX cc, RENAME INDEX cc TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX cc, RENAME INDEX cc TO bb;

-- echo # combination: aaab

-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i1, RENAME INDEX i1 TO i2;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i1, RENAME INDEX i1 TO bb;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i1, RENAME INDEX i1 TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX aa, RENAME INDEX aa TO bb;

# Remaining combinations: abcd abab acbc cacb ccab

-- echo # combination: abcd

-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i2, RENAME INDEX cc TO i4;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i2, RENAME INDEX cc TO dd;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i2, RENAME INDEX cc TO i4;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i2, RENAME INDEX cc TO dd;

# add existing, rename to existing
-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i2, RENAME INDEX i3 TO i4;
# add existing
-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i2, RENAME INDEX i3 TO dd;
# rename to existing
-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i2, RENAME INDEX i3 TO i4;

-- enable_info
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i2, RENAME INDEX i3 TO dd;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

-- echo # combination: abab

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- enable_info
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i2, RENAME INDEX i1 TO i2;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i1(f), DROP INDEX bb, RENAME INDEX i1 TO bb;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i2, RENAME INDEX aa TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX bb, RENAME INDEX aa TO bb;

-- echo # combination: acbc

-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i1(f), DROP INDEX cc, RENAME INDEX i2 TO cc;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX cc, RENAME INDEX i2 TO cc;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i1(f), DROP INDEX cc, RENAME INDEX bb TO cc;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX aa(f), DROP INDEX cc, RENAME INDEX bb TO cc;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i3, RENAME INDEX bb TO i3;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i3, RENAME INDEX bb TO i3;

# add existing
-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX i1(f), DROP INDEX i3, RENAME INDEX i2 TO i3;

-- enable_info
ALTER TABLE t ADD INDEX aa(f), DROP INDEX i3, RENAME INDEX i2 TO i3;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

-- echo # combination: cacb

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX cc(f), DROP INDEX i1, RENAME INDEX cc TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX cc(f), DROP INDEX aa, RENAME INDEX cc TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX cc(f), DROP INDEX aa, RENAME INDEX cc TO bb;
-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX cc(f), DROP INDEX i1, RENAME INDEX cc TO bb;

-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX i3(f), DROP INDEX i1, RENAME INDEX i3 TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i3(f), DROP INDEX aa, RENAME INDEX i3 TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i3(f), DROP INDEX aa, RENAME INDEX i3 TO bb;

-- enable_info
ALTER TABLE t ADD INDEX i3(f), DROP INDEX i1, RENAME INDEX i3 TO bb;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

-- echo # combination: ccab

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX cc(f), DROP INDEX cc, RENAME INDEX i1 TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX cc(f), DROP INDEX cc, RENAME INDEX i1 TO bb;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX cc(f), DROP INDEX cc, RENAME INDEX aa TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX cc(f), DROP INDEX cc, RENAME INDEX aa TO bb;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i3(f), DROP INDEX cc, RENAME INDEX aa TO i2;
-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i3(f), DROP INDEX cc, RENAME INDEX aa TO bb;

-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX i3(f), DROP INDEX i3, RENAME INDEX i1 TO i2;

-- enable_info
ALTER TABLE t ADD INDEX i3(f), DROP INDEX i3, RENAME INDEX i1 TO bb;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
# A simple successful ALTER
-- enable_info
ALTER TABLE t RENAME INDEX i1 TO x;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- error ER_DUP_KEYNAME
ALTER TABLE t RENAME INDEX i1 TO i2;

-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t RENAME INDEX foo TO i1;

# Test ADD INDEX, RENAME INDEX

-- enable_info
ALTER TABLE t ADD INDEX i9 (f), RENAME INDEX i1 TO i8;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- enable_info
ALTER TABLE t ADD INDEX i1 (f), RENAME INDEX i1 TO i9;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;
-- eval $create
-- eval $insert

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- error ER_DUP_KEYNAME
ALTER TABLE t ADD INDEX foo (f), RENAME INDEX i1 TO foo;

# Test ADD INDEX, RENAME INDEX, DROP INDEX

-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t ADD INDEX i1 (f), RENAME INDEX i1 TO foo, DROP INDEX i1;

-- error ER_CANT_DROP_FIELD_OR_KEY
ALTER TABLE t ADD INDEX i1 (f), RENAME INDEX i1 TO foo, DROP INDEX foo;

-- error ER_CANT_DROP_FIELD_OR_KEY
# "ALTER TABLE t ADD INDEX foo (d), DROP INDEX foo;" alone fails with the
# same error code, but we have that test here anyway
ALTER TABLE t ADD INDEX foo (f), RENAME INDEX foo TO bar, DROP INDEX foo;

# Test RENAME INDEX, RENAME INDEX

-- error ER_DUP_KEYNAME
ALTER TABLE t RENAME INDEX i1 TO x, RENAME INDEX i2 TO x;

-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t RENAME INDEX i1 TO x, RENAME INDEX i1 TO y;

-- error ER_KEY_DOES_NOT_EXITS
ALTER TABLE t RENAME INDEX i1 TO x, RENAME INDEX i1 TO x;

# show that the table did not change after all the erroneous ALTERs
-- eval $show_table
-- eval $show_sys
-- eval $show_stats

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;

# now test the rebuild case (new clustered index)

CREATE TABLE t (
	c1 INT NOT NULL,
	c2 INT NOT NULL,
	c3 INT,
	c4 INT,
	PRIMARY KEY (c1),
	INDEX i1 (c3),
	INDEX i2 (c4)
) ENGINE=INNODB;

INSERT INTO t SET c1=1, c2=2;

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- enable_info
ALTER TABLE t DROP PRIMARY KEY, ADD PRIMARY KEY (c2), RENAME INDEX i1 TO x;
-- disable_info

-- eval $show_table
-- eval $show_sys
-- eval $show_stats

-- enable_info
ALTER TABLE t RENAME INDEX i2 TO y, ROW_FORMAT=REDUNDANT;
-- disable_info

-- eval $show_table
-- eval $show_sys
-- eval $show_stats

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;

# a case where the PK does not exist prior to the ALTER TABLE command

CREATE TABLE t (
	c1 INT NOT NULL,
	c2 INT,
	c3 INT,
	INDEX i1 (c2),
	INDEX i2 (c3)
) ENGINE=INNODB;

INSERT INTO t SET c1=1;

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- enable_info
ALTER TABLE t ADD PRIMARY KEY (c1), RENAME INDEX i1 TO x;
-- disable_info
-- eval $show_table
-- eval $show_sys
-- eval $show_stats

--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;

# Test repeated RENAMEs with alternating names

CREATE TABLE t (a INT, INDEX iiiii (a)) ENGINE=INNODB;
INSERT INTO t SET a=NULL;
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_alter.inc
-- enable_info
ALTER TABLE t RENAME INDEX iiiii TO i;
ALTER TABLE t RENAME INDEX i TO iiiii;
ALTER TABLE t RENAME INDEX iiiii TO i;
ALTER TABLE t RENAME INDEX i TO iiiii;
-- disable_info
--source $MYSQLTEST_VARDIR/tmp/set_osu_method_for_other_ddls.inc
DROP TABLE t;

# Below is a shell script to generate the full set of ALTER TABLE
# DROP/ADD/RENAME combinations. The generated .sql file is 3.3MB and
# executes in about 7 minutes.
#
##!/bin/sh
#
#create="
#CREATE TABLE t (
#        a INT,
#        b INT,
#        c INT,
#        d INT,
#        PRIMARY KEY (a),
#        INDEX i1 (b),
#        INDEX i2 (c)
#) ENGINE=INNODB;
#"
#
#echo "DROP TABLE IF EXISTS t;"
#for r in "" ", DROP PRIMARY KEY, ADD PRIMARY KEY (a)" ", ROW_FORMAT=REDUNDANT" ; do
#    for i1 in i1 i1noexist; do
#        for i2 in i2 i2noexist; do
#            for i3 in i3 i3noexist; do
#                for i4 in i4 i4noexist; do
#                    for a in $i1 $i2 $i3 $i4; do
#                        for b in $i1 $i2 $i3 $i4; do
#                            for c in $i1 $i2 $i3 $i4; do
#                                for d in $i1 $i2 $i3 $i4; do
#                                    echo "$create"
#                                    echo "ALTER TABLE t ADD INDEX $a (d), RENAME INDEX $b TO $c, DROP INDEX $d $r;"
#                                    echo "DROP TABLE t;"
#                                done
#                            done
#                        done
#                    done
#                done
#            done
#        done
#    done
#done
