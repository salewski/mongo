
/**
 *    Copyright (C) 2018-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/ops/delete.h"

#include "mongo/db/catalog/database.h"
#include "mongo/db/exec/delete.h"
#include "mongo/db/ops/parsed_delete.h"
#include "mongo/db/query/get_executor.h"
#include "mongo/db/repl/repl_client_info.h"

namespace mongo {

long long deleteObjects(OperationContext* opCtx,
                        Collection* collection,
                        const NamespaceString& ns,
                        BSONObj pattern,
                        bool justOne,
                        bool god,
                        bool fromMigrate) {
    DeleteRequest request(ns);
    request.setQuery(pattern);
    request.setMulti(!justOne);
    request.setGod(god);
    request.setFromMigrate(fromMigrate);

    ParsedDelete parsedDelete(opCtx, &request);
    uassertStatusOK(parsedDelete.parseRequest());

    auto exec = uassertStatusOK(
        getExecutorDelete(opCtx, &CurOp::get(opCtx)->debug(), collection, &parsedDelete));

    uassertStatusOK(exec->executePlan());

    return DeleteStage::getNumDeleted(*exec);
}

boost::optional<BSONObj> deleteObject(OperationContext* opCtx,
                                      Collection* collection,
                                      const DeleteRequest& request) {
    ParsedDelete parsedDelete(opCtx, &request);
    uassertStatusOK(parsedDelete.parseRequest());

    // This method doesn't support multi-deletes.
    invariant(!request.isMulti());

    auto exec = uassertStatusOK(
        getExecutorDelete(opCtx, &CurOp::get(opCtx)->debug(), collection, &parsedDelete));

    BSONObj docImage;
    if (exec->getNext(&docImage, nullptr) == PlanExecutor::IS_EOF) {
        return boost::none;
    }

    return {request.shouldReturnDeleted() ? docImage.getOwned() : BSONObj()};
}

}  // namespace mongo
