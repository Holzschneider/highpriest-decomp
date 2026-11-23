-- Development accounts for a private test server.
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`, `group_id`) VALUES
  (2000100, 'bfadmin', 'bfadmin', 'M', 'admin@localhost', 99),
  (2000101, 'bftest',  'bftest',  'F', 'test@localhost',   0)
ON DUPLICATE KEY UPDATE `userid` = `userid`;
