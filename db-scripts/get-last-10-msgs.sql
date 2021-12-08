SELECT username, message, created_at FROM (
	SELECT * FROM message LEFT JOIN user
	WHERE message.from_user == user.uid
	AND message.is_dm == 1
	ORDER BY id
	DESC LIMIT 15
) ORDER BY id ASC