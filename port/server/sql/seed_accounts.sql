-- Development accounts for a private test server.
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`, `group_id`) VALUES
  (2000100, 'hpadmin', 'hpadmin', 'M', 'admin@localhost', 99),
  (2000101, 'hptest',  'hptest',  'F', 'test@localhost',   0)
ON DUPLICATE KEY UPDATE `userid` = `userid`;
