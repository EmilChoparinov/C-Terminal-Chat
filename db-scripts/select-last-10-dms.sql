SELECT message, created_at, "from", "to" FROM (
	SELECT
	message.id, message, created_at,
	user_from.username AS "from", user_to.username as "to"
	FROM message
	LEFT JOIN user user_from
	ON message.from_user == user_from.uid
	LEFT JOIN user user_to
	ON message.to_user == user_to.uid
	WHERE message.is_dm == 0
	AND (message.from_user == 1 OR message.from_user == 4)
	AND (message.to_user == 1 OR message.to_user == 4)
	ORDER BY message.id DESC
	LIMIT 15
) ORDER BY id ASC