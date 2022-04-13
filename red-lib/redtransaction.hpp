/*
 Copyright 2021 IoT.bzh

 author: Clément Bénier <clement.benier@iot.bzh>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */


#ifndef REDLIB_REDTRANSACTION_HPP
#define REDLIB_REDTRANSACTION_HPP

#include <libdnf/base/transaction.hpp>

namespace redlib {

class RedTransaction: public libdnf::base::Transaction {
public:
	RedTransaction(libdnf::base::Transaction transaction): libdnf::base::Transaction(transaction) {}
	std::vector<libdnf::base::TransactionPackage> & get_transaction_packages();	
private:
	std::vector<libdnf::base::TransactionPackage> transaction_packages;
};

} //namespace redlib
#endif
