#include "fdacdaohub1.hpp"
#include "functions.cpp"
#include "deposits.cpp"
#include "components.cpp"

ACTION fdacdaohub1::setgrpstate(name groupname, uint8_t newstate){
  require_auth(get_self() );
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr != _groups.end(), "Group not found.");
  check(group_itr->state != newstate, "Group already in this state.");
  _groups.modify( group_itr, groupname, [&]( auto& a) {
      a.state = newstate;
  });
}
ACTION fdacdaohub1::setsettings(settings new_settings, bool remove){
  require_auth(get_self());
  settings_table _settings(get_self(), get_self().value);
  if(remove){
    _settings.remove();
    return;
  }
  _settings.set(new_settings, get_self());
}

ACTION fdacdaohub1::versioning(name modulename, checksum256 codehash, checksum256 abihash, string json_src, string info, uint64_t update_key){
  require_auth(get_self() );
  checksum256 test;
  check(modulename.value, "Must specify a modulename");
  versions_table _versions(get_self(), modulename.value);

  if(update_key){
    auto existing_version = _versions.find(update_key);
    check(existing_version != _versions.end(), "Version with this key doesn't exist, can't update.");

    if(codehash==test && abihash==test && json_src.empty() && info.empty() ){
      _versions.erase(existing_version);
      return;
    }
    _versions.modify( existing_version, same_payer, [&]( auto& v) {
      v.codehash = codehash == test ? v.codehash : codehash;
      v.abihash = abihash == test ? v.abihash : abihash;
      v.json_src = json_src.empty() ? v.json_src : json_src;
      v.info = info.empty() ? v.info : info;
    });
    return;
  }
  check(codehash != test && abihash != test && !json_src.empty(), "codehash, abihash and json_src required.");
  uint64_t id = _versions.available_primary_key()==0 ? 1 : _versions.available_primary_key();
  _versions.emplace(get_self(), [&](auto& v) {
      v.version = id;
      v.codehash = codehash;
      v.abihash = codehash;
      v.json_src = json_src;
      v.info = info;
  });
}

ACTION fdacdaohub1::versionstate(name modulename, uint64_t version, uint8_t status ){
  require_auth(get_self() );
  check(modulename.value, "Must specify a modulename.");
  versions_table _versions(get_self(), modulename.value);
  auto itr = _versions.find(version);
  check(itr != _versions.end(), "Version doesn't exists.");
  _versions.modify( itr, same_payer, [&]( auto& v) {
    v.status = status;
  });
}

ACTION fdacdaohub1::creategroup(name groupname, name creator, asset resource_estimation) {

  require_auth(creator);
  check(!is_account(groupname), "The chosen accountname is already taken.");
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr == _groups.end(), "group already registered as a group.");

    // Create record if it does not exist
  _groups.emplace(creator, [&](auto& group) {
    group.groupname = groupname;
    group.creator = creator;
  });

  create_group_account(groupname, creator, resource_estimation);

}

ACTION fdacdaohub1::linkgroup(name groupname, name creator, groupmeta meta, uiconf ui, uint8_t state, vector<name> tags, uint64_t claps, time_point_sec creation_date) {

  require_auth(get_self() );
  check(is_account(groupname), "The group account doesn't exist.");
  check(is_account(creator), "The creator account doesn't exist.");

  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr == _groups.end(), "Account already registered as a group.");

    // Create record if it does not exist
  _groups.emplace(get_self(), [&](auto& group) {
    group.groupname = groupname;
    group.creator = creator;
    group.meta = meta;
    group.ui = ui;
    group.state = state;
    group.tags = tags;
    group.claps = claps;
    group.creation_date = creation_date;
  });
}



ACTION fdacdaohub1::activate(name groupname, name creator) {
  require_auth(creator);
  groups_table _groups(get_self(), get_self().value);

  auto group_itr = _groups.find(groupname.value);

  check(group_itr != _groups.end(), "Group not found.");
  check(group_itr->creator == creator, "Only the group creator can activate.");
  check(group_itr->state == 0, "Group already activated.");

  _groups.modify( group_itr, same_payer, [&]( auto& a) {
      a.state = 1;
  });

  action(
    permission_level{ groupname, "owner"_n },
    groupname,
    "invitecust"_n,
    std::make_tuple(creator)
  ).send();

}


ACTION fdacdaohub1::updateabout(name groupname, string about){
  require_auth(groupname);
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr != _groups.end(), "Group not found.");
  check(group_itr->state != 0, "Group isn't active yet.");
  _groups.modify( group_itr, groupname, [&]( auto& a) {
      a.meta.about = about;
  });
}
ACTION fdacdaohub1::updatetitle(name groupname, string title){
  require_auth(groupname);
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr != _groups.end(), "Group not found.");
  check(group_itr->state != 0, "Group isn't active yet.");
  _groups.modify( group_itr, groupname, [&]( auto& a) {
      a.meta.title = title;
  });
}

ACTION fdacdaohub1::updatelinks(name groupname, vector <link> newlinks){
  require_auth(groupname);
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr != _groups.end(), "Group not found.");
  check(group_itr->state != 0, "Group isn't active yet.");
  _groups.modify( group_itr, groupname, [&]( auto& a) {
      a.meta.links = newlinks;
  });
}

ACTION fdacdaohub1::updatelogo(name groupname, string logo){
  require_auth(groupname);
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr != _groups.end(), "Group not found.");
  check(group_itr->state != 0, "Group isn't active yet.");
  _groups.modify( group_itr, groupname, [&]( auto& a) {
      a.ui.logo = logo;
  });
}

ACTION fdacdaohub1::updatecolor(name groupname, string hexcolor){
  require_auth(groupname);
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr != _groups.end(), "Group not found.");
  check(group_itr->state != 0, "Group isn't active yet.");
  _groups.modify( group_itr, groupname, [&]( auto& a) {
      a.ui.hexcolor = hexcolor;
  });
}

ACTION fdacdaohub1::setcustomui(name groupname, string url){
  require_auth(groupname);
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr != _groups.end(), "Group not found.");
  check(group_itr->state != 0, "Group isn't active yet.");
  if(!url.empty() ){
    check(url.rfind("https://", 0) == 0, "Url must be https://");
  }
  _groups.modify( group_itr, groupname, [&]( auto& a) {
      a.ui.custom_ui_url = url;
  });
}

ACTION fdacdaohub1::updatetags(name groupname, vector<name> tags){
  require_auth(groupname);
  check(tags.size() <= 6, "Too many tags, max 6.");
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr != _groups.end(), "Group not found.");
  check(group_itr->state != 0, "Group isn't active yet.");
  //remove dups
  tags.erase( unique( tags.begin(), tags.end() ), tags.end() );
  for(name const& tag: tags) {
    //validate individual tags here
    check(tag.value != 0, "invalid tag name");
  }
  _groups.modify( group_itr, groupname, [&]( auto& a) {
      a.tags = tags;
  });
}

ACTION fdacdaohub1::messagebus(name sender_group, name event, string message, vector<name> receivers){
  //pass data through the chain but doesn't store it.
  //only active groups are allowed to use the message bus
  require_auth(sender_group);
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(sender_group.value);
  check(group_itr != _groups.end(), "Invalid sender.");
  check(group_itr->state != 0, "Inactive group can't send messsages.");
}


ACTION fdacdaohub1::unlinkgroup(name groupname) {
  //get_self is authorized to delete groups from the hub! The group will still exist though.
  //however deleted groups can not be found via the ui. Direct links will still work.
  check(eosio::has_auth(groupname) || eosio::has_auth(get_self()), "Not authorized to unlink the group from the hub.");
  groups_table _groups(get_self(), get_self().value);
  auto group_itr = _groups.find(groupname.value);
  check(group_itr != _groups.end(), "group doesn't exist.");
  _groups.erase(group_itr);

}


ACTION fdacdaohub1::clear() {
  //maintenance actions
  
  groups_table _groups(get_self(), get_self().value);
  // Delete all records in table
  auto itr = _groups.begin();
  while (itr != _groups.end()) {
    itr = _groups.erase(itr);
  }
}

//notify transfer handler
void fdacdaohub1::on_transfer(name from, name to, asset quantity, string memo){

  if(get_first_receiver() != name("eosio.token") || quantity.symbol.code() != symbol_code("EOS") ){
    return;
  }

  if (from == get_self() || to != get_self()) {
    return;
  }
  if ( from == name("eosio") || from == name("eosio.bpay") ||
       from == name("eosio.msig") || from == name("eosio.names") ||
       from == name("eosio.prods") || from == name("eosio.ram") ||
       from == name("eosio.ramfee") || from == name("eosio.saving") ||
       from == name("eosio.stake") || from == name("eosio.token") ||
       from == name("eosio.unregd") || from == name("eosio.vpay") ||
       from == name("eosio.wrap") || from == name("eosio.rex") ) {
    return;
  }

  if(memo.substr(0, 16) == "clap for group: " ){

    name potentialgroupname = memo.length() >= 17 ? name(memo.substr(16, 12 ) ) : name(0);
    check(potentialgroupname != name(0), "No group name in memo");
    groups_table _groups(get_self(), get_self().value);
    auto itr = _groups.find(potentialgroupname.value);
    check(itr != _groups.end(), "Group does not exists.");
    check(itr->state != 0, "Group is not activated.");
    _groups.modify( itr, same_payer, [&]( auto& n) {
        n.claps += quantity.amount;
    });
    //add to contract balance
    add_deposit(get_self(), extended_asset(quantity, get_first_receiver()) );
    return;
  }

  add_deposit(from, extended_asset(quantity, get_first_receiver()) );
}


















ACTION fdacdaohub1::migrategrps (uint8_t batch, uint8_t skip){
/*
  require_auth(get_self() );
  groups2_table _source(get_self(), get_self().value);
  groups_table _destination(get_self(), get_self().value);

  //groups_table _source(get_self(), get_self().value);
 // groups2_table _destination(get_self(), get_self().value);
  

  auto src_itr = _source.begin();
  while (src_itr != _source.end()) {


    _destination.emplace(get_self(), [&](auto& g) {
      g.groupname= src_itr->groupname;
      g.creator= src_itr->creator;
      g.meta= src_itr->meta;
      g.ui= src_itr->ui;
      g.state= src_itr->state;
      g.r1= src_itr->r1;
      g.r2= src_itr->r2;
      g.creation_date= src_itr->creation_date;
      g.tags = src_itr->tags;
      g.flag =src_itr->flag;
      g.claps=src_itr->claps;
    });
    src_itr = _source.erase(src_itr);
  }
  
*/

}



