WITH catCount AS (
    SELECT C.iid, COUNT(*) AS count
    FROM Categories C
    GROUP BY C.iid
)
SELECT COUNT(C.iid)
FROM catCount C
WHERE C.count == 4;