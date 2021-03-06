#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>

#include "config.h"
#include "pow10.h"
#include "str_expand.h"
#include "symbols.h"

class crowdsale : public eosio::contract {
private:
	struct state_t {
		eosio::extended_asset total_eos;
		eosio::asset total_usd;
		eosio::asset usdoneth;
		eosio::asset eosusd;
		eosio::asset usdtkn;
		int32_t valid_until;
		bool finalized;
		bool finished;
		bool hardcap_reached;
		time_t start;
		time_t finish;
#ifdef DEBUG
		time_t time;
#endif
	};

	struct deposit_t {
		account_name account;
		eosio::asset usd;
		eosio::extended_asset eos;
		eosio::extended_asset tkn;
		uint64_t primary_key() const { return account; }
	};

	struct userlist_t {
		account_name account;
		uint64_t primary_key() const { return account; }
	};

	typedef eosio::multi_index<N(deposit), deposit_t> deposit;
	eosio::singleton<N(state), state_t> state_singleton;
	eosio::multi_index<N(whitelist), userlist_t> whitelist;

	account_name issuer;

	state_t state;

	void on_deposit(account_name investor, eosio::extended_asset quantity);

	state_t default_parameters() const {
		return state_t{
			.total_eos = ASSET_EOS(0),
			.total_usd = ASSET_USD(0),
			.usdoneth = ASSET_USD(0),
			.eosusd = ASSET_USD(0),
			.usdtkn = ASSET_TKN(0),
			.valid_until = 0,
			.finalized = false,
			.finished = false,
			.hardcap_reached = false,
			.start = 0,
			.finish = 0,
#ifdef DEBUG
			.time = 0
#endif
		};
	}

	inline eosio::asset eos2usd(eosio::asset asset_eos, eosio::asset eosusd) const {
		return ASSET_USD(static_cast<int64_t>(static_cast<int128_t>(asset_eos.amount) * eosusd.amount / POW10(4)));
	}

	inline eosio::extended_asset usd2eos(eosio::asset asset_usd, eosio::asset eosusd) const {
		return ASSET_EOS(asset_usd.amount * POW10(4) / eosusd.amount);
	}

	inline eosio::extended_asset usd2tkn(eosio::asset asset_usd, eosio::asset usdtkn) const {
		return ASSET_TKN(static_cast<int64_t>(static_cast<int128_t>(asset_usd.amount) * usdtkn.amount / POW10(DECIMALS)));
	}

	inline eosio::asset total_usd() const {
		return this->state.total_usd + this->state.usdoneth;
	}

	void inline_issue(account_name to, eosio::extended_asset quantity, std::string memo) const {
		struct issue {
			account_name to;
			eosio::asset quantity;
			std::string memo;
		};
		eosio::action(
			eosio::permission_level(this->_self, N(active)),
			quantity.contract,
			N(issue),
			issue{to, quantity, memo}
		).send();
	}

	void inline_transfer(account_name from, account_name to, eosio::extended_asset quantity, std::string memo) const {
		struct transfer {
			account_name from;
			account_name to;
			eosio::asset quantity;
			std::string memo;
		};
		eosio::action(
			eosio::permission_level(this->_self, N(active)),
			quantity.contract,
			N(transfer),
			transfer{from, to, quantity, memo}
		).send();
	}

	void setwhite(account_name account) {
		auto it = this->whitelist.find(account);
		eosio_assert(it == this->whitelist.end(), "Account already whitelisted");
		this->whitelist.emplace(this->_self, [account](auto& e) {
			e.account = account;
		});

		deposit deposit_table(this->_self, this->_self);
		auto deposit_it = deposit_table.find(account);
		if (deposit_it != deposit_table.end()) {
			this->state.total_eos += deposit_it->eos;
			this->state.total_usd += deposit_it->usd;
		}
	}

public:
	crowdsale(account_name self);
	~crowdsale();
	void transfer(uint64_t sender, uint64_t receiver);
	void init(time_t start, time_t finish);
	void setstart(time_t start);
	void setfinish(time_t finish);
	void white(account_name account);
	void whitemany(eosio::vector<account_name> accounts);
	void withdraw(account_name investor);
	void refund(account_name investor);
	void finalize();
	void setdaily(eosio::asset usdoneth, eosio::asset eosusd, eosio::asset usdtkn, time_t next_update);
#ifdef DEBUG
	void settime(time_t time);
#endif
};
