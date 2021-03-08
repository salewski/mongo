/**
 * Test to ensure that specifying non-local read concern with an uninitiated set does not crash
 * node.
 *
 * @tags: [requires_persistence, requires_majority_read_concern]
 */
(function() {
"use strict";

const rst = new ReplSetTest({nodes: 1});
rst.startSet();
const localDB = rst.nodes[0].getDB('local');
assert.commandWorked(localDB.test.insert({_id: 0}));
assert.commandWorked(localDB.runCommand({
    hello: 1,
    "$clusterTime": {
        "clusterTime": Timestamp(1, 1),
        "signature": {"hash": BinData(0, "AAAAAAAAAAAAAAAAAAAAAAAAAAA="), "keyId": NumberLong(0)}
    }
}));
jsTestLog("Local readConcern on local database should work.");
const res = assert.commandWorked(localDB.runCommand(
    {find: "test", filter: {}, maxTimeMS: 60000, readConcern: {level: "local"}}));
assert.eq([{_id: 0}], res.cursor.firstBatch);

jsTestLog("Majority readConcern should fail with NotYetInitialized.");
assert.commandFailedWithCode(
    localDB.runCommand(
        {find: "test", filter: {}, maxTimeMS: 60000, readConcern: {level: "majority"}}),
    ErrorCodes.NotYetInitialized);

// TODO SERVER-47568: Only expect NotPrimaryOrSecondary.
const expectedCodes = jsTest.options().useRandomBinVersionsWithinReplicaSet
    ? [ErrorCodes.NotYetInitialized, ErrorCodes.NotPrimaryOrSecondary]
    : [ErrorCodes.NotYetInitialized];

jsTestLog("afterClusterTime readConcern should fail with NotYetInitialized.");
assert.commandFailedWithCode(localDB.runCommand({
    find: "test",
    filter: {},
    maxTimeMS: 60000,
    readConcern: {afterClusterTime: Timestamp(1, 1)}
}),
                             expectedCodes);

jsTestLog("oplog query should fail with NotYetInitialized.");
assert.commandFailedWithCode(localDB.runCommand({
    find: "oplog.rs",
    filter: {ts: {$gte: Timestamp(1520004466, 2)}},
    tailable: true,
    awaitData: true,
    maxTimeMS: 60000,
    batchSize: 13981010,
    term: 1,
    readConcern: {afterClusterTime: Timestamp(1, 1)}
}),
                             expectedCodes);
rst.stopSet();
}());
