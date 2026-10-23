-- MySQL dump 10.13  Distrib 8.0.46, for Linux (x86_64)
--
-- Host: localhost    Database: yuuye_DB
-- ------------------------------------------------------
-- Server version	8.0.46-0ubuntu0.24.04.4

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `interview_room`
--

DROP TABLE IF EXISTS `interview_room`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `interview_room` (
  `room_id` int NOT NULL AUTO_INCREMENT,
  `room_name` varchar(50) NOT NULL,
  `location` varchar(100) DEFAULT NULL,
  `capacity` int NOT NULL DEFAULT '1',
  `status` tinyint NOT NULL DEFAULT '1' COMMENT '1可预约,0停用',
  PRIMARY KEY (`room_id`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `interview_room`
--

LOCK TABLES `interview_room` WRITE;
/*!40000 ALTER TABLE `interview_room` DISABLE KEYS */;
INSERT INTO `interview_room` VALUES (1,'面试间1','一楼会议室',1,1),(2,'面试间2','二楼会议室',1,1);
/*!40000 ALTER TABLE `interview_room` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `reservation_info`
--

DROP TABLE IF EXISTS `reservation_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `reservation_info` (
  `reservation_id` bigint NOT NULL AUTO_INCREMENT,
  `userid` int NOT NULL,
  `room_id` int NOT NULL,
  `reserve_date` date NOT NULL,
  `start_slot` tinyint unsigned NOT NULL COMMENT '开始时段[',
  `end_slot` tinyint unsigned NOT NULL COMMENT '结束边界)',
  `status` tinyint NOT NULL DEFAULT '1' COMMENT '1已预约，2已完成，3已取消，4未到场',
  `checkin_at` datetime DEFAULT NULL COMMENT '实际开始使用时间',
  `checkout_at` datetime DEFAULT NULL COMMENT '实际结束使用时间',
  `created_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`reservation_id`),
  UNIQUE KEY `uk_reservation_room_date` (`reservation_id`,`room_id`,`reserve_date`),
  KEY `idx_user_date` (`userid`,`reserve_date`),
  KEY `idx_room_date` (`room_id`,`reserve_date`),
  CONSTRAINT `fk_reservation_room` FOREIGN KEY (`room_id`) REFERENCES `interview_room` (`room_id`)
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `reservation_info`
--

LOCK TABLES `reservation_info` WRITE;
/*!40000 ALTER TABLE `reservation_info` DISABLE KEYS */;
INSERT INTO `reservation_info` VALUES (1,3,1,'2026-10-18',2,4,1,NULL,NULL,'2026-10-17 14:54:33');
/*!40000 ALTER TABLE `reservation_info` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `room_slot`
--

DROP TABLE IF EXISTS `room_slot`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `room_slot` (
  `room_id` int NOT NULL,
  `reserve_date` date NOT NULL,
  `slot_no` tinyint unsigned NOT NULL,
  `reservation_id` bigint NOT NULL,
  PRIMARY KEY (`room_id`,`reserve_date`,`slot_no`),
  KEY `idx_slot_reservation` (`reservation_id`,`room_id`,`reserve_date`),
  CONSTRAINT `fk_slot_reservation` FOREIGN KEY (`reservation_id`, `room_id`, `reserve_date`) REFERENCES `reservation_info` (`reservation_id`, `room_id`, `reserve_date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `room_slot`
--

LOCK TABLES `room_slot` WRITE;
/*!40000 ALTER TABLE `room_slot` DISABLE KEYS */;
INSERT INTO `room_slot` VALUES (1,'2026-10-18',2,1),(1,'2026-10-18',3,1);
/*!40000 ALTER TABLE `room_slot` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `user_info`
--

DROP TABLE IF EXISTS `user_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!50503 SET character_set_client = utf8mb4 */;
CREATE TABLE `user_info` (
  `userid` int NOT NULL AUTO_INCREMENT,
  `tel` char(11) NOT NULL,
  `username` varchar(20) NOT NULL,
  `passwd` varchar(8) NOT NULL,
  `status` tinyint NOT NULL,
  PRIMARY KEY (`userid`),
  UNIQUE KEY `userid` (`userid`),
  UNIQUE KEY `tel` (`tel`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `user_info`
--

LOCK TABLES `user_info` WRITE;
/*!40000 ALTER TABLE `user_info` DISABLE KEYS */;
INSERT INTO `user_info` VALUES (1,'1310000000','小王','123456',1),(2,'1320000000','小明','123456',1),(3,'17313780363','xibei','123456',1);
/*!40000 ALTER TABLE `user_info` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2026-10-22 19:52:20
